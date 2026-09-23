"""Create a deterministic Vast 3.2.16 source bundle without signing secrets."""

import hashlib
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "dist" / "Vast-v3.2.16-source.zip"
PREFIX = "Vast-v3.2.16-source/"
FIXED_TIME = (2026, 9, 23, 0, 0, 0)
EXCLUDED_TOP_LEVEL = {"artifacts", "build", "dist", "testdata30", "testdata_canvas_ocr"}
EXCLUDED_DIRS = {".git", ".vs", "__pycache__", ".pytest_cache"}
INCLUDED_TOP_LEVEL_FILES = {
    "RELEASE_3.2.16.md", "RELEASE_3.2.14.md", "UI_REVIEW_3.2.14.md",
    "build-native.sh", "README.md", "IMPLEMENTATION_REPORT.md", "RELEASE_3.2.10.md", "RELEASE_3.2.11.md", "RELEASE_3.2.12.md", "RELEASE_3.2.13.md", "MENU_REVIEW.md", "UI_REVIEW_3.2.12.md",
    "THIRD_PARTY_NOTICES.md", "test_canvas_ocr.c", "test_ocr.c",
    "test_ppocr_recognizer.c", "test_v30.c",
}
INCLUDED_TOP_LEVEL_DIRS = {"assets", "src", "stubs", "tests", "third_party", "tools"}
GENERATED_SUFFIXES = {".apk", ".aab", ".exe", ".pyc"}
SECRET_SUFFIXES = {".pem", ".pfx", ".p12", ".jks", ".keystore", ".pk8", ".key"}
SECRET_NAMES = {".env", "id_rsa", "id_ed25519", "credentials.json", "service-account.json", "local.properties"}
SECRET_NAME_FRAGMENTS = {"private-key", "private_key", "service-account", "credentials"}
SECRET_CONTENT_MARKERS = (
    b"-----BEGIN " + b"PRIVATE " + b"KEY-----",
    b"-----BEGIN RSA " + b"PRIVATE " + b"KEY-----",
    b"-----BEGIN EC " + b"PRIVATE " + b"KEY-----",
    b"-----BEGIN OPENSSH " + b"PRIVATE " + b"KEY-----",
    b'"private_' + b'key":',
    b'"client_' + b'secret":',
    b'"refresh_' + b'token":',
)


def included(path):
    relative = path.relative_to(ROOT)
    lower_name = path.name.lower()
    if relative.parts[0] in EXCLUDED_TOP_LEVEL:
        return False
    if relative.parts[0] not in INCLUDED_TOP_LEVEL_DIRS and relative.as_posix() not in INCLUDED_TOP_LEVEL_FILES:
        return False
    if any(part in EXCLUDED_DIRS for part in relative.parts[:-1]):
        return False
    if path.is_symlink():
        raise RuntimeError(f"refusing to package symbolic link: {relative}")
    if path.suffix.lower() in GENERATED_SUFFIXES:
        return False
    if (path.suffix.lower() in SECRET_SUFFIXES or lower_name in SECRET_NAMES
            or lower_name.startswith(".env.")
            or any(fragment in lower_name for fragment in SECRET_NAME_FRAGMENTS)):
        raise RuntimeError(f"refusing to package possible signing secret: {relative}")
    return True


def reject_secret_content(relative, data):
    if any(marker in data for marker in SECRET_CONTENT_MARKERS):
        raise RuntimeError(f"refusing to package possible secret content: {relative}")


def zip_info(name, mode=0o644):
    info = zipfile.ZipInfo(name, FIXED_TIME)
    info.compress_type = zipfile.ZIP_DEFLATED
    info.create_system = 3
    info.external_attr = (0o100000 | mode) << 16
    return info


def main():
    files = sorted(
        (path for path in ROOT.rglob("*") if path.is_file() and included(path)),
        key=lambda path: path.relative_to(ROOT).as_posix(),
    )
    manifest_lines = []
    OUTPUT.parent.mkdir(exist_ok=True)
    with zipfile.ZipFile(OUTPUT, "w", allowZip64=True, compresslevel=9) as archive:
        for path in files:
            relative = path.relative_to(ROOT).as_posix()
            data = path.read_bytes()
            reject_secret_content(relative, data)
            manifest_lines.append(f"{hashlib.sha256(data).hexdigest()}  {relative}")
            mode = 0o755 if relative == "build-native.sh" else 0o644
            archive.writestr(zip_info(PREFIX + relative, mode), data)
        manifest = ("\n".join(manifest_lines) + "\n").encode("utf-8")
        archive.writestr(zip_info(PREFIX + "SOURCE_ARCHIVE_MANIFEST.sha256"), manifest)

    with zipfile.ZipFile(OUTPUT) as archive:
        names = archive.namelist()
        corrupt = archive.testzip()
        if corrupt is not None:
            raise RuntimeError(f"source archive contains a corrupt member: {corrupt}")
        if len(names) != len(set(names)):
            raise RuntimeError("source archive contains duplicate member names")
        if any(name.lower().endswith(tuple(SECRET_SUFFIXES)) for name in names):
            raise RuntimeError("source archive contains a secret-like suffix")
    digest = hashlib.sha256(OUTPUT.read_bytes()).hexdigest()
    print(
        f"{OUTPUT} files={len(files) + 1} bytes={OUTPUT.stat().st_size} "
        f"sha256={digest} private_key_entries=0 zip_test=PASS"
    )


if __name__ == "__main__":
    main()
