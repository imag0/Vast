"""Generate deterministic handwriting-like OCR fixtures and optionally probe ONNX.

The generated PGM files are test inputs, not application assets.  They use
handwritten/calligraphic fonts, per-glyph baseline/angle variation, shear, and
light antialias blur so the recognition test is not a perfect typeset sample.
"""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont


CASES = {
    "zh_engine": ("发动机", "zh"),
    "zh_school": ("明天去学校", "zh"),
    "en_terms": ("engine pressure navigation", "en"),
    "mixed": ("Engine 发动机", "mixed"),
}


def glyph_font(kind: str, ch: str, en_font: Path, zh_font: Path, size: int):
    font_path = zh_font if kind == "zh" or ord(ch) > 127 else en_font
    return ImageFont.truetype(str(font_path), size=size)


def render_handwriting(text: str, kind: str, en_font: Path, zh_font: Path) -> Image.Image:
    size = 72 if kind == "zh" else 68
    glyphs: list[tuple[str, ImageFont.FreeTypeFont, int, int, float]] = []
    total_w = 28
    for i, ch in enumerate(text):
        font = glyph_font(kind, ch, en_font, zh_font, size)
        box = font.getbbox(ch if ch != " " else "n")
        width = (box[2] - box[0]) if ch != " " else max(18, (box[2] - box[0]) // 2)
        y = int(round(4.2 * math.sin(i * 1.73) + ((i * 7) % 3 - 1)))
        angle = 1.8 * math.sin(i * 1.19)
        glyphs.append((ch, font, width, y, angle))
        total_w += width + (2 if ord(ch) > 127 else 0)

    canvas = Image.new("L", (total_w + 32, 112), 255)
    x = 18
    for ch, font, width, yoff, angle in glyphs:
        if ch == " ":
            x += width
            continue
        tile = Image.new("L", (width + 40, 104), 255)
        draw = ImageDraw.Draw(tile)
        box = draw.textbbox((0, 0), ch, font=font, stroke_width=0)
        draw.text((18 - box[0], 13 - box[1]), ch, font=font, fill=18)
        tile = tile.rotate(angle, resample=Image.Resampling.BICUBIC, expand=False, fillcolor=255)
        canvas.paste(tile, (x - 18, yoff), tile.point(lambda v: 255 - v))
        x += width + (2 if ord(ch) > 127 else 0)

    # Small affine shear and optical blur imitate a captured vector raster.
    shear = 0.035 if kind != "zh" else -0.025
    canvas = canvas.transform(
        canvas.size,
        Image.Transform.AFFINE,
        (1.0, shear, -shear * 45.0, 0.0, 1.0, 0.0),
        resample=Image.Resampling.BICUBIC,
        fillcolor=255,
    ).filter(ImageFilter.GaussianBlur(0.38))
    bbox = canvas.point(lambda v: 0 if v > 247 else 255).getbbox()
    if bbox:
        canvas = canvas.crop((max(0, bbox[0] - 10), max(0, bbox[1] - 10),
                              min(canvas.width, bbox[2] + 10), min(canvas.height, bbox[3] + 10)))
    return canvas


def preprocess(image: Image.Image) -> np.ndarray:
    rgb = np.repeat(np.asarray(image.convert("L"), dtype=np.uint8)[..., None], 3, axis=2)
    h, w = rgb.shape[:2]
    max_ratio = max(320.0 / 48.0, w / float(h))
    input_w = min(int(48.0 * max_ratio), 3200)
    resized_w = min(int(math.ceil(48.0 * w / float(h))), input_w)
    resized = Image.fromarray(rgb, mode="RGB").resize((resized_w, 48), Image.Resampling.BILINEAR)
    chw = np.asarray(resized, dtype=np.float32).transpose(2, 0, 1) / 127.5 - 1.0
    tensor = np.zeros((1, 3, 48, input_w), dtype=np.float32)
    tensor[0, :, :, :resized_w] = chw
    return tensor


def dictionary(path: Path) -> list[str]:
    chars = path.read_text(encoding="utf-8").splitlines()
    return [""] + chars + [" "]


def decode(logits: np.ndarray, chars: list[str]) -> tuple[str, float]:
    raw = logits[0].argmax(axis=1)
    probs = logits[0].max(axis=1)
    result: list[str] = []
    kept: list[float] = []
    previous = -1
    for idx, prob in zip(raw.tolist(), probs.tolist()):
        if idx != previous and idx != 0:
            result.append(chars[idx])
            kept.append(float(prob))
        previous = idx
    return "".join(result), (sum(kept) / len(kept) if kept else 0.0)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=Path("tests/fixtures/ocr"))
    parser.add_argument("--english-font", type=Path,
                        default=Path(r"C:\Windows\Fonts\Inkfree.ttf"))
    parser.add_argument("--chinese-font", type=Path,
                        default=Path(r"C:\Windows\Fonts\simkai.ttf"))
    parser.add_argument("--model", type=Path)
    parser.add_argument("--dict", dest="dict_path", type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)

    session = None
    chars = None
    if args.model:
        import onnxruntime as ort

        session = ort.InferenceSession(str(args.model), providers=["CPUExecutionProvider"])
        if not args.dict_path:
            raise SystemExit("--dict is required with --model")
        chars = dictionary(args.dict_path)

    report: dict[str, object] = {}
    for name, (text, kind) in CASES.items():
        image = render_handwriting(text, kind, args.english_font, args.chinese_font)
        path = args.output / f"{name}.pgm"
        image.save(path)
        image.save(args.output / f"{name}.png")
        entry: dict[str, object] = {
            "source_text": text,
            "width": image.width,
            "height": image.height,
        }
        if session is not None and chars is not None:
            input_meta = session.get_inputs()[0]
            output_meta = session.get_outputs()[0]
            logits = session.run([output_meta.name], {input_meta.name: preprocess(image)})[0]
            recognized, confidence = decode(logits, chars)
            entry.update(recognized=recognized, confidence=confidence,
                         input_name=input_meta.name, output_name=output_meta.name,
                         input_width=int(preprocess(image).shape[3]))
            print(f"{name}: {recognized!r} confidence={confidence:.6f} "
                  f"raster={image.width}x{image.height}")
        report[name] = entry
    (args.output / "reference-results.json").write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
