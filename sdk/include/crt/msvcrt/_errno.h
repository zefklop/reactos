/**
 * This file has no copyright assigned and is placed in the Public Domain.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _CRT_ERRNO_DEFINED
#  define _CRT_ERRNO_DEFINED
_CRTIMP extern int *__cdecl _errno(void);
#  define errno (*_errno())

  errno_t __cdecl _set_errno(_In_ int _Value);
  errno_t __cdecl _get_errno(_Out_ int *_Value);
#endif

#ifdef __cplusplus
}
#endif
