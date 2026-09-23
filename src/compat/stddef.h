#ifndef VAST_COMPAT_STDDEF_H
#define VAST_COMPAT_STDDEF_H
typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __WCHAR_TYPE__ wchar_t;
typedef struct { long long __align; long double __ld; } max_align_t;
#ifndef NULL
#define NULL ((void *)0)
#endif
#define offsetof(type, member) __builtin_offsetof(type, member)
#endif
