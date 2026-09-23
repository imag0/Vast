# Vast third-party notices

Vast bundles the following third-party components for fully offline handwriting recognition. The files listed below are shipped locally; no model or runtime download is needed after installation.

## PP-OCRv5 mobile text-recognition model

- Component: `PP-OCRv5_mobile_rec`
- Project/source: [PaddleOCR](https://github.com/PaddlePaddle/PaddleOCR)
- Official model documentation: <https://www.paddleocr.ai/latest/en/version3.x/module_usage/text_recognition.html>
- Official inference archive: <https://paddle-model-ecology.bj.bcebos.com/paddlex/official_inference_model/paddle3.0.0/PP-OCRv5_mobile_rec_infer.tar>
- Original archive: 16,834,560 bytes; SHA-256 `566b9512b34e34a9f0db54d87b51fa5a0b9ed2cf1ab7e49728cc0b8b5a64f414`
- Converted ONNX model: 16,537,720 bytes; SHA-256 `dc7de8ee31d9246783cf346f3b207b4cb871e11824b1cfab2ff20aa1f1f8e67b`
- Inference configuration SHA-256: `5dfeb2777f6d0db8177d8128a8acfcf6e6276dc4ac73ea3bf0dc06d6a5e85d8e`
- Character dictionary SHA-256: `d1979e9f794c464c0d2e0b70a7fe14dd978e9dc644c0e71f14158cdf8342af1b`
- License: Apache License 2.0. The license text is preserved at `third_party/paddleocr/LICENSE` and in the APK at `assets/licenses/paddleocr-LICENSE`.

The official Paddle inference model was converted to ONNX with PaddlePaddle 3.1.1 and Paddle2ONNX 2.1.0 at ONNX opset 11:

```text
paddle2onnx --model_dir PP-OCRv5_mobile_rec_infer --model_filename inference.json --params_filename inference.pdiparams --save_file inference.onnx --opset_version 11 --enable_auto_update_opset True --enable_onnx_checker True --optimize_tool None --enable_verbose False
```

Two clean conversions were byte-identical. The conversion tool itself is not included in the application.

## Paddle2ONNX

- Version used for conversion: 2.1.0
- Project/source: <https://github.com/PaddlePaddle/Paddle2ONNX>
- License: Apache License 2.0. The license text supplied with Paddle2ONNX 2.1.0 is preserved at `third_party/paddle2onnx/LICENSE` and in the APK at `assets/licenses/paddle2onnx-LICENSE`.

## ONNX Runtime for Android

- Component/version: ONNX Runtime for Android 1.28.0
- Project/source: <https://github.com/microsoft/onnxruntime>
- Maven artifact: <https://repo1.maven.org/maven2/com/microsoft/onnxruntime/onnxruntime-android/1.28.0/onnxruntime-android-1.28.0.aar>
- Original AAR: 45,634,470 bytes; SHA-256 `f351a0638696f54b35184290dbc001d66daae17281ad0b548d2c70347d53b8a9`
- Shipped library: `arm64-v8a/libonnxruntime.so`, 28,637,280 bytes; SHA-256 `f826d8efb03adf0a84f10e7ba408f9d4cd11b0a2ccd8d08aeb0f7451fb50cacc`
- License: MIT. The license text is preserved at `third_party/onnxruntime/LICENSE` and in the APK at `assets/licenses/onnxruntime-LICENSE`.
- ONNX Runtime's complete bundled third-party notices are preserved verbatim at `third_party/onnxruntime/ThirdPartyNotices.txt` and in the APK at `assets/licenses/onnxruntime-ThirdPartyNotices.txt`.

Only ONNX Runtime's `arm64-v8a` native library is packaged. The Java/JNI wrapper and the `armeabi-v7a`, `x86`, and `x86_64` binaries from the AAR are not shipped.
