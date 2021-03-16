#pragma once

#ifdef __cplusplus
extern "C" {
#endif

_Check_return_
_CRTIMP
wint_t
__cdecl
towupper(_In_ wint_t _C);

_Check_return_
_CRTIMP
wint_t
__cdecl
towlower(_In_ wint_t _C);

/* Not in libntoskrnl */
#ifndef _CRT_IS_NTOS

_Check_return_
_CRTIMP
int
__cdecl
iswctype(
    _In_ wint_t _C,
    _In_ wctype_t _Type);

_Check_return_
_CRTIMP
int
__cdecl
iswdigit(_In_ wint_t _C);

_Check_return_
_CRTIMP
int
__cdecl
iswxdigit(_In_ wint_t _C);

_Check_return_
_CRTIMP
int
__cdecl
iswspace(_In_ wint_t _C);

_Check_return_
_CRTIMP
int
__cdecl
iswalpha(_In_ wint_t _C);

#endif /* !defined(_CRT_IS_NTOS) */

#ifdef __cplusplus
}
#endif
