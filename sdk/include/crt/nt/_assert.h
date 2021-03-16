
#pragma once

#ifndef NDEBUG

#include <rtlfuncs.h>

#  ifndef assert
#    define assert(_Expression) (void)((!!(_Expression)) || (RtlAssert(_CRT_STRINGIZE(_Expression), (void*)__FILE__,__LINE__, NULL),0))
#  endif

/* No unicode version of RtlAssert */
#  ifndef wassert
#    define wassert(_Expression) (void)((!!(_Expression)) || (RtlAssert(_CRT_STRINGIZE(_Expression), (void*)__FILE__,__LINE__, NULL),0))
#  endif
#else
#  ifndef assert
#    define assert(_Expression) ((void)0)
#  endif

#  ifndef wassert
#    define wassert(_Expression) ((void)0)
#  endif
#endif /* defined(NDEBUG) */
