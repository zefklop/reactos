/**
 * This file has no copyright assigned and is placed in the Public Domain.
 */

#pragma once

#define _FPCLASS_SNAN   0x0001	/* Signaling "Not a Number" */
#define _FPCLASS_QNAN   0x0002	/* Quiet "Not a Number" */
#define _FPCLASS_NINF   0x0004	/* Negative Infinity */
#define _FPCLASS_NN     0x0008	/* Negative Normal */
#define _FPCLASS_ND     0x0010	/* Negative Denormal */
#define _FPCLASS_NZ     0x0020	/* Negative Zero */
#define _FPCLASS_PZ     0x0040	/* Positive Zero */
#define _FPCLASS_PD     0x0080	/* Positive Denormal */
#define _FPCLASS_PN     0x0100	/* Positive Normal */
#define _FPCLASS_PINF   0x0200	/* Positive Infinity */

#if !defined(_M_IX86) && !defined(_M_AMD64)
/* FIXME: Check that definitions below are correct for more architectures */
#error "Check me!"
#endif

#define FLT_RADIX 2

#define FLT_MANT_DIG 24
#define DBL_MANT_DIG 53
#define LDBL_MANT_DIG DBL_MANT_DIG

#define FLT_DIG 6
#define DBL_DIG 15
#define LDBL_DIG 15

#define FLT_MIN_EXP (-125)
#define DBL_MIN_EXP (-1021)
#define LDBL_MIN_EXP (-1021)

#define FLT_MAX_EXP 128
#define DBL_MAX_EXP 1024
#define LDBL_MAX_EXP 1024

#define FLT_MIN_10_EXP (-37)
#define DBL_MIN_10_EXP (-307)
#define LDBL_MIN_10_EXP (-307)

#define FLT_MAX_10_EXP 38
#define DBL_MAX_10_EXP 308
#define LDBL_MAX_10_EXP 308

#define FLT_MIN 1.175494351e-38F
#define DBL_MIN 2.2250738585072014e-308
#define LDBL_MIN 2.2250738585072014e-308

#define FLT_MAX 3.402823466e+38F
#define DBL_MAX 1.7976931348623158e+308
#define LDBL_MAX 1.7976931348623158e+308

#define FLT_EPSILON 1.192092896e-07F
#define DBL_EPSILON 2.2204460492503131e-016
#define LDBL_EPSILON 2.2204460492503131e-016

/* Control word values */
#define _EM_INEXACT     0x00000001
#define _EM_UNDERFLOW   0x00000002
#define _EM_OVERFLOW    0x00000004
#define _EM_ZERODIVIDE  0x00000008
#define _EM_INVALID     0x00000010
#define _EM_DENORMAL    0x00080000
#define _IC_AFFINE      0x00040000
#define _IC_PROJECTIVE  0x00000000
#define _RC_CHOP        0x00000300
#define _RC_UP          0x00000200
#define _RC_DOWN        0x00000100
#define _RC_NEAR        0x00000000
#define _PC_24          0x00020000
#define _PC_53          0x00010000
#define _PC_64          0x00000000

/* Control word masks */
#define _MCW_EM     0x0008001F  /* Error masks */
#define _MCW_IC     0x00040000  /* Infinity */
#define _MCW_RC     0x00000300  /* Rounding */
#define _MCW_PC     0x00030000  /* Precision */
#define _MCW_DN     0x03000000  /* Denormal */

#ifdef __cplusplus
extern "C" {
#endif

_Check_return_
__MINGW_NOTHROW
_CRTIMP
int
__cdecl
_finite(_In_ double);

_Check_return_
__MINGW_NOTHROW
_CRTIMP
int
__cdecl
_isnan(_In_ double);

_Check_return_
__MINGW_NOTHROW
_CRTIMP
int
__cdecl
_fpclass(_In_ double);

_Check_return_
__MINGW_NOTHROW
_CRTIMP
double
__cdecl
_scalb(_In_ double, _In_ long);

#if defined(_M_IX86) || defined(_M_AMD64)
__MINGW_NOTHROW
_CRTIMP
unsigned int
__cdecl
_control87(
  _In_ unsigned int unNew,
  _In_ unsigned int unMask);
#endif

#ifdef __cplusplus
} // extern "C"
#endif
