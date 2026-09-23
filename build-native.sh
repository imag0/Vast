#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
cd "$ROOT"

CLANG="${CLANG:-clang}"
TARGET="${VAST_ANDROID_TARGET:-aarch64-linux-android26}"

mkdir -p build/stubs

STUB_FLAGS="--target=$TARGET -shared -fPIC -nostdlib"
"$CLANG" $STUB_FLAGS stubs/android_stub.c -Wl,-soname,libandroid.so -o build/stubs/libandroid.so
"$CLANG" $STUB_FLAGS stubs/libc_stub.c -Wl,-soname,libc.so -o build/stubs/libc.so
"$CLANG" $STUB_FLAGS stubs/libdl_stub.c -Wl,-soname,libdl.so -o build/stubs/libdl.so
"$CLANG" $STUB_FLAGS stubs/libm_stub.c -Wl,-soname,libm.so -o build/stubs/libm.so
"$CLANG" $STUB_FLAGS stubs/egl_stub.c -Wl,-soname,libEGL.so -o build/stubs/libEGL.so
"$CLANG" $STUB_FLAGS stubs/gles2_stub.c -Wl,-soname,libGLESv2.so -o build/stubs/libGLESv2.so

OCR_SOURCES="
src/ocr_hash.c
src/ocr_identity.c
src/ocr_segmenter.c
src/ocr_rasterizer.c
src/handwriting_index.c
src/ocr_sidecar.c
src/ocr_unicode_android.c
src/ppocr_recognizer.c
"
if [ -f src/ocr_manager.c ]; then
    OCR_SOURCES="$OCR_SOURCES src/ocr_manager.c"
fi

"$CLANG" --target="$TARGET" -DVAST_GPU -DVAST_OCR -O3 -ffast-math \
    -fno-math-errno -fomit-frame-pointer -shared -fPIC -nostdlib \
    -Isrc/compat -Isrc -Ithird_party/onnxruntime/include \
    -Wl,-soname,libcanvas.so -Wl,-z,defs -Wl,-z,max-page-size=16384 -Lbuild/stubs \
    src/canvas.c $OCR_SOURCES \
    -landroid -lEGL -lGLESv2 -lc -lm -ldl \
    -o build/libcanvas.so
printf 'Built build/libcanvas.so\n'
