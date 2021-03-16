
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _CRT_CTYPEDATA_DEFINED
#  define _CRT_CTYPEDATA_DEFINED
_CRTIMP const unsigned short * __cdecl __pctype_func(void);
_CRTIMP const unsigned short *_pctype;

#  ifndef _CTYPE_DISABLE_MACROS
#    ifndef __PCTYPE_FUNC
#      ifdef _DLL
#        define __PCTYPE_FUNC __pctype_func()
#      else
#        define __PCTYPE_FUNC _pctype
#      endif
#    endif /* !__PCTYPE_FUNC */

#    ifdef _DLL
#      define _pctype (__pctype_func())
#    endif /* _M_CEE_PURE */
#  endif /* !_CTYPE_DISABLE_MACROS */

#endif /* !_CRT_CTYPEDATA_DEFINED */

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
_isctype(
    _In_ int _C,
    _In_ int _Type);

_Check_return_
_CRTIMP
int
__cdecl
isalnum(_In_ int _C);

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
isupper(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
isalpha(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
isprint(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
isgraph(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
iscntrl(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
islower(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
ispunct(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
isxdigit(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
__iscsymf(_In_ int _C);

_Check_return_
_CRTIMP
int
__cdecl
__iscsym(_In_ int _C);

#  ifndef NO_OLDNAMES

_Check_return_
_CRTIMP
_CRT_NONSTDC_DEPRECATE(__isascii)
_CRTIMP
int
__cdecl
isascii(_In_ int _C);

#  endif /* !defined(NO_OLDNAMES) */

/* For compatibility with libstdc++ */
#  if defined(__cplusplus) && (__cplusplus >= 201103L)
__forceinline int isblank (int _C)
{
    return (_isctype(_C, _BLANK) || _C == '\t');
}
#  endif /* defined(__cplusplus) && (__cplusplus >= 201103L) */

/* Not exported in win2K3. Have it available when building it though */
#  if defined(_MSVCRT_) || (defined(_DLL) && _WIN32_WINNT >= 0x600)
_Check_return_
_CRTIMP
int
__cdecl
_isctype_l(
    _In_ int _C,
    _In_ int _Type,
    _In_opt_ _locale_t _Locale);
#  endif /* defined(_MSVCRT_) || (defined(_DLL) && _WIN32_WINNT >= 0x600) */

#endif /* !defined(_CTYPE_DEFINED) */

#ifdef __cplusplus
} // extern "C"
#endif
