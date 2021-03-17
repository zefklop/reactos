
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

_CRTIMP extern const unsigned short _wctype[];
_CRTIMP const wctype_t * __cdecl __pwctype_func(void);
_CRTIMP extern const wctype_t *_pwctype;

#  ifndef _CTYPE_DISABLE_MACROS
#    define _pwctype (__pwctype_func())
#  endif /* !_CTYPE_DISABLE_MACROS */

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

_Check_return_
_CRTIMP
int
__cdecl
iswalpha(_In_ wint_t _C);

_Check_return_
_CRTIMP
int
__cdecl
iswascii(_In_ wint_t _C);

_Check_return_
_CRTIMP
int
__cdecl
iswupper(_In_ wint_t _C);

_Check_return_
_CRTIMP
int
__cdecl
iswlower(_In_ wint_t _C);

_Check_return_
_CRTIMP
int
__cdecl
iswpunct(_In_ wint_t _C);

_Check_return_
_CRTIMP
int
__cdecl
iswalnum(_In_ wint_t _C);

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
isleadbyte(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
iswprint(_In_ wint_t _C);

_Check_return_
_CRTIMP
int
__cdecl
iswcntrl(_In_ wint_t _C);

_Check_return_
_CRTIMP
int
__cdecl
iswgraph(_In_ wint_t _C);

/* For compatibility with libstdc++ */
#  if defined(__cplusplus) && (__cplusplus >= 201103L)
__forceinline int iswblank(_In_ wint_t _C)
{
    return (iswctype(_C, 0x40 /* _BLANK */) || _C == '\t');
}
#  endif /* defined(__cplusplus) && (__cplusplus >= 201103L) */

/* Not exported in win2K3. Have it available when building it though */
#  if defined(_MSVCRT_) || (defined(_DLL) && _WIN32_WINNT >= 0x600)
_Check_return_
_CRTIMP
int
__cdecl
_iswctype_l(
    _In_ wint_t _C,
    _In_ wctype_t _Type,
    _In_opt_ _locale_t _Locale);

_Check_return_
_CRTIMP
int
__cdecl
_isleadbyte_l(
    _In_ int _C,
    _In_opt_ _locale_t _Locale);

#  endif /* defined(_MSVCRT_) || (defined(_DLL) && _WIN32_WINNT >= 0x600) */

#ifdef __cplusplus
}
#endif
