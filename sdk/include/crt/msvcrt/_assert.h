
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

_CRTIMP
void
__cdecl
_assert(
    _In_z_ const char *_Message,
    _In_z_ const char *_File,
    _In_ unsigned _Line);

_CRTIMP
void
__cdecl
_wassert(
    _In_z_ const wchar_t *_Message,
    _In_z_ const wchar_t *_File,
    _In_ unsigned _Line);

#ifdef __cplusplus
} // extern "C"
#endif

#ifndef NDEBUG
#  ifndef assert
#    define assert(_Expression) (void)((!!(_Expression)) || (_assert(#_Expression,__FILE__,__LINE__),0))
#  endif

#  ifndef wassert
#    define wassert(_Expression) (void)((!!(_Expression)) || (_wassert(_CRT_WIDE(#_Expression),_CRT_WIDE(__FILE__),__LINE__),0))
#  endif
#else
#  ifndef assert
#    define assert(_Expression) ((void)0)
#  endif

#  ifndef wassert
#    define wassert(_Expression) ((void)0)
#  endif
#endif /* defined(NDEBUG) */
