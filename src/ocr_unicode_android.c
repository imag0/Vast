#include "ocr_unicode_android.h"
#include "jni.h"

extern void *malloc(OcrSize);
extern void free(void *);

static int clear_exception(JNIEnv *env) {
    if (!env || !(*env)->ExceptionCheck(env)) return 0;
    (*env)->ExceptionClear(env);
    return 1;
}

static int decode_utf8(const char *src, OcrU32 len, jchar **out,
                       jsize *out_len) {
    OcrU32 i = 0, n = 0;
    jchar *buf;
    if (!src || !out || !out_len || !ocr_utf8_valid(src, len))
        return OCR_ERR_INVALID;
    if (len > 0x3fffffffu) return OCR_ERR_LIMIT;
    buf = (jchar *)malloc(((OcrSize)len + 1u) * sizeof(jchar));
    if (!buf) return OCR_ERR_NOMEM;
    while (i < len) {
        OcrU32 cp, c = (OcrU8)src[i++];
        if (c < 0x80u) cp = c;
        else if ((c & 0xe0u) == 0xc0u) {
            OcrU32 c1 = (OcrU8)src[i++];
            cp = ((c & 0x1fu) << 6) | (c1 & 0x3fu);
        } else if ((c & 0xf0u) == 0xe0u) {
            OcrU32 c1 = (OcrU8)src[i++];
            OcrU32 c2 = (OcrU8)src[i++];
            cp = ((c & 0x0fu) << 12) | ((c1 & 0x3fu) << 6) |
                 (c2 & 0x3fu);
        } else {
            OcrU32 c1 = (OcrU8)src[i++];
            OcrU32 c2 = (OcrU8)src[i++];
            OcrU32 c3 = (OcrU8)src[i++];
            cp = ((c & 0x07u) << 18) | ((c1 & 0x3fu) << 12) |
                 ((c2 & 0x3fu) << 6) | (c3 & 0x3fu);
        }
        if (cp <= 0xffffu) buf[n++] = (jchar)cp;
        else {
            cp -= 0x10000u;
            buf[n++] = (jchar)(0xd800u + (cp >> 10));
            buf[n++] = (jchar)(0xdc00u + (cp & 0x3ffu));
        }
    }
    *out = buf;
    *out_len = (jsize)n;
    return OCR_OK;
}

static int unicode_space(OcrU32 cp) {
    return (cp >= 0x09u && cp <= 0x0du) || cp == 0x20u || cp == 0x85u ||
           cp == 0xa0u || cp == 0x1680u ||
           (cp >= 0x2000u && cp <= 0x200au) || cp == 0x2028u ||
           cp == 0x2029u || cp == 0x202fu || cp == 0x205fu || cp == 0x3000u;
}

static OcrU32 utf8_width(OcrU32 cp) {
    return cp < 0x80u ? 1u : (cp < 0x800u ? 2u : (cp < 0x10000u ? 3u : 4u));
}

static void emit_utf8(char *dst, OcrU32 *at, OcrU32 cp) {
    OcrU32 p = *at;
    if (cp < 0x80u) dst[p++] = (char)cp;
    else if (cp < 0x800u) {
        dst[p++] = (char)(0xc0u | (cp >> 6));
        dst[p++] = (char)(0x80u | (cp & 0x3fu));
    } else if (cp < 0x10000u) {
        dst[p++] = (char)(0xe0u | (cp >> 12));
        dst[p++] = (char)(0x80u | ((cp >> 6) & 0x3fu));
        dst[p++] = (char)(0x80u | (cp & 0x3fu));
    } else {
        dst[p++] = (char)(0xf0u | (cp >> 18));
        dst[p++] = (char)(0x80u | ((cp >> 12) & 0x3fu));
        dst[p++] = (char)(0x80u | ((cp >> 6) & 0x3fu));
        dst[p++] = (char)(0x80u | (cp & 0x3fu));
    }
    *at = p;
}

static OcrU32 utf16_cp(const jchar *s, OcrU32 n, OcrU32 *at) {
    OcrU32 a = *at, cp = s[a++];
    if (cp >= 0xd800u && cp <= 0xdbffu && a < n) {
        OcrU32 lo = s[a];
        if (lo >= 0xdc00u && lo <= 0xdfffu) {
            ++a;
            cp = 0x10000u + ((cp - 0xd800u) << 10) + (lo - 0xdc00u);
        }
    }
    *at = a;
    return cp;
}

static int encode_normalized(JNIEnv *env, jstring text, char **out,
                             OcrU32 *out_len) {
    const jchar *chars;
    jsize jn;
    OcrU32 i = 0, bytes = 0, at = 0;
    int pending_space = 0, emitted = 0;
    char *dst;
    if (!text || !out || !out_len) return OCR_ERR_INVALID;
    jn = (*env)->GetStringLength(env, text);
    chars = (*env)->GetStringChars(env, text, 0);
    if (!chars || clear_exception(env)) return OCR_ERR_INVALID;
    while (i < (OcrU32)jn) {
        OcrU32 cp = utf16_cp(chars, (OcrU32)jn, &i);
        if (unicode_space(cp)) {
            if (emitted) pending_space = 1;
        } else {
            if (pending_space) ++bytes;
            pending_space = 0;
            bytes += utf8_width(cp);
            emitted = 1;
            if (bytes > OCR_MAX_TEXT_BYTES) {
                (*env)->ReleaseStringChars(env, text, chars);
                return OCR_ERR_LIMIT;
            }
        }
    }
    dst = (char *)malloc((OcrSize)bytes + 1u);
    if (!dst) {
        (*env)->ReleaseStringChars(env, text, chars);
        return OCR_ERR_NOMEM;
    }
    i = 0; pending_space = 0; emitted = 0;
    while (i < (OcrU32)jn) {
        OcrU32 cp = utf16_cp(chars, (OcrU32)jn, &i);
        if (unicode_space(cp)) {
            if (emitted) pending_space = 1;
        } else {
            if (pending_space) emit_utf8(dst, &at, 0x20u);
            pending_space = 0;
            emit_utf8(dst, &at, cp);
            emitted = 1;
        }
    }
    dst[at] = 0;
    (*env)->ReleaseStringChars(env, text, chars);
    *out = dst;
    *out_len = at;
    return OCR_OK;
}

int ocr_unicode_normalize_android(void *jni_env, const char *text, OcrU32 len,
                                  char **out_text, OcrU32 *out_len) {
    JNIEnv *env = (JNIEnv *)jni_env;
    jchar *wide = 0;
    jsize wide_len = 0;
    jstring input = 0, nfkc = 0, lower = 0;
    jclass form_class = 0, norm_class = 0, string_class = 0, locale_class = 0;
    jobject form = 0, root = 0;
    jfieldID form_id, root_id;
    jmethodID norm_id, lower_id;
    int rc;
    if (!out_text || !out_len) return OCR_ERR_INVALID;
    *out_text = 0; *out_len = 0;
    if (!env) return ocr_normalize_basic(text, len, out_text, out_len);
    rc = decode_utf8(text, len, &wide, &wide_len);
    if (rc != OCR_OK) return rc;
    input = (*env)->NewString(env, wide, wide_len);
    free(wide); wide = 0;
    if (!input || clear_exception(env)) { rc = OCR_ERR_INVALID; goto done; }
    form_class = (*env)->FindClass(env, "java/text/Normalizer$Form");
    norm_class = (*env)->FindClass(env, "java/text/Normalizer");
    string_class = (*env)->FindClass(env, "java/lang/String");
    locale_class = (*env)->FindClass(env, "java/util/Locale");
    if (!form_class || !norm_class || !string_class || !locale_class ||
        clear_exception(env)) { rc = OCR_ERR_INVALID; goto done; }
    form_id = (*env)->GetStaticFieldID(env, form_class, "NFKC",
                                      "Ljava/text/Normalizer$Form;");
    root_id = (*env)->GetStaticFieldID(env, locale_class, "ROOT",
                                      "Ljava/util/Locale;");
    norm_id = (*env)->GetStaticMethodID(
        env, norm_class, "normalize",
        "(Ljava/lang/CharSequence;Ljava/text/Normalizer$Form;)Ljava/lang/String;");
    lower_id = (*env)->GetMethodID(env, string_class, "toLowerCase",
                                   "(Ljava/util/Locale;)Ljava/lang/String;");
    if (!form_id || !root_id || !norm_id || !lower_id || clear_exception(env)) {
        rc = OCR_ERR_INVALID; goto done;
    }
    form = (*env)->GetStaticObjectField(env, form_class, form_id);
    root = (*env)->GetStaticObjectField(env, locale_class, root_id);
    nfkc = (jstring)(*env)->CallStaticObjectMethod(env, norm_class, norm_id,
                                                   input, form);
    if (!nfkc || clear_exception(env)) { rc = OCR_ERR_INVALID; goto done; }
    lower = (jstring)(*env)->CallObjectMethod(env, nfkc, lower_id, root);
    if (!lower || clear_exception(env)) { rc = OCR_ERR_INVALID; goto done; }
    rc = encode_normalized(env, lower, out_text, out_len);
done:
    if (lower) (*env)->DeleteLocalRef(env, lower);
    if (nfkc) (*env)->DeleteLocalRef(env, nfkc);
    if (root) (*env)->DeleteLocalRef(env, root);
    if (form) (*env)->DeleteLocalRef(env, form);
    if (locale_class) (*env)->DeleteLocalRef(env, locale_class);
    if (string_class) (*env)->DeleteLocalRef(env, string_class);
    if (norm_class) (*env)->DeleteLocalRef(env, norm_class);
    if (form_class) (*env)->DeleteLocalRef(env, form_class);
    if (input) (*env)->DeleteLocalRef(env, input);
    return rc;
}

static int bytes_contains(const char *hay, const char *needle) {
    OcrU32 i, j;
    if (!needle || !needle[0]) return 1;
    if (!hay) return 0;
    for (i = 0; hay[i]; ++i) {
        for (j = 0; needle[j] && hay[i + j] == needle[j]; ++j) {}
        if (!needle[j]) return 1;
    }
    return 0;
}

int ocr_unicode_contains_android(void *jni_env, const char *haystack,
                                 const char *needle) {
    char *h = 0, *n = 0;
    OcrU32 hl = 0, nl = 0;
    int found = 0;
    if (!needle || !needle[0]) return 1;
    if (!haystack) return 0;
    if (ocr_unicode_normalize_android(jni_env, haystack,
                                      (OcrU32)__builtin_strlen(haystack),
                                      &h, &hl) != OCR_OK) goto done;
    if (ocr_unicode_normalize_android(jni_env, needle,
                                      (OcrU32)__builtin_strlen(needle),
                                      &n, &nl) != OCR_OK) goto done;
    found = bytes_contains(h, n);
done:
    if (h) free(h);
    if (n) free(n);
    (void)hl; (void)nl;
    return found;
}
