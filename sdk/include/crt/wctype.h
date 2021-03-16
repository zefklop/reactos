/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_WCTYPE
#define _INC_WCTYPE

#include <crtdefs.h>

/* For compatibility with libstdc++. NOT IMPLEMENTED! */
#  if defined(__cplusplus) && (__cplusplus >= 201103L)
typedef wchar_t wctrans_t;
wint_t __cdecl towctrans(wint_t,wctrans_t);
wctrans_t __cdecl wctrans(const char *);
wctype_t __cdecl wctype(const char *);
#  endif

#include __crt_bits_header(_wctype.h)

#endif
