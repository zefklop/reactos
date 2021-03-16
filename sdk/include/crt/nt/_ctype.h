
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _CTYPE_DEFINED

_Check_return_
_CRTIMP
int
__cdecl
toupper(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
tolower(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
isspace(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
isdigit(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
isxdigit(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
isupper(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
islower(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
isprint(_In_ int _C);

/* Not in ntoskrnl */
#ifndef _CRT_IS_NTOS
_Check_return_
_CRTIMP
int
__cdecl
isalpha(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
iscntrl(_In_ int _C);

#endif /* !defined(_CRT_IS_NTOS) */

/* Only available in libcntpr */
#ifdef _CRT_IS_LIBCNT
_Check_return_
_CRTIMP
int
__cdecl
_isctype(
    _In_ int _C,
    _In_ int _Type);
#endif

#endif /* !defined(_CTYPE_DEFINED) */

#ifdef __cplusplus
} // extern "C"
#endif
