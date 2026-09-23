#ifndef VAST_OCR_UNICODE_ANDROID_H
#define VAST_OCR_UNICODE_ANDROID_H

#include "ocr_core.h"

/*
 * Normalize standard UTF-8 with Android's platform Unicode implementation:
 * NFKC, Locale.ROOT lowercase, and collapsed Unicode whitespace.  The caller
 * owns *out_text and releases it with free().  Passing a null JNI environment
 * uses the small host-test fallback from ocr_core.
 */
int ocr_unicode_normalize_android(void *jni_env, const char *text, OcrU32 len,
                                  char **out_text, OcrU32 *out_len);
int ocr_unicode_contains_android(void *jni_env, const char *haystack,
                                 const char *needle);

#endif
