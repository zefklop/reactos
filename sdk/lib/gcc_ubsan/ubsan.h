
#include <stdint.h>
#include <inttypes.h>

struct SourceLocation {
  const char *Filename;
  uint32_t Line;
  uint32_t Column;
};

struct TypeDescriptor {
    uint16_t TypeKind;
    uint16_t TypeInfo;
    char TypeName[1];
};

#ifndef _UBSAN_NOBREAK_
#define _UBSAN_BREAK() __asm__("int $3")
#else
#define _UBSAN_BREAK()
#endif

#ifdef _UBSAN_MSVCRT_

#include <windef.h>
#include <winbase.h>
#include <stdio.h>

#define ubsan_fail(SourceLoc, error, fmt, ...) do {     \
    char buffer[256];                                   \
    OutputDebugStringA("UB SANITIZER: " error " at ");  \
    OutputDebugStringA((SourceLoc)->Filename);          \
    _snprintf(buffer, sizeof(buffer), ":%ld:%ld - " fmt "\n", (SourceLoc)->Line, (SourceLoc)->Column, ##__VA_ARGS__);  \
    OutputDebugStringA(buffer); \
    _UBSAN_BREAK();          \
} while(0)

#elif defined(_UBSAN_WIN32K_)

#include <windef.h>
#include <wingdi.h>
#include <winddi.h>
#include <stdarg.h>

static inline
void
ubsan_fail_helper(char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    EngDebugPrint("UB SANITIZER", fmt, args);
    va_end(args);

    _UBSAN_BREAK();
}

#define ubsan_fail(SourceLoc, error, fmt, ...) ubsan_fail_helper(error " at %s:%d:%d - " fmt "\n", (SourceLoc)->Filename, (SourceLoc)->Line, (SourceLoc)->Column, ##__VA_ARGS__)

#elif defined(_UBSAN_SCSIPORT_)

#include <ntddk.h>
#include <srb.h>

#define ubsan_fail(SourceLoc, error, fmt, ...) do {     \
    ScsiDebugPrint(0, "UB SANITIZER: " error " at %s:%d:%d - " fmt "\n", (SourceLoc)->Filename, (SourceLoc)->Line, (SourceLoc)->Column, ##__VA_ARGS__);    \
    _UBSAN_BREAK();  \
} while(0)

#elif defined(_UBSAN_VIDEOPRT_)

#include <ntdef.h>
#include <miniport.h>
#include <video.h>

#define ubsan_fail(SourceLoc, error, fmt, ...) do {     \
    VideoPortDebugPrint(0, "UB SANITIZER: " error " at %s:%d:%d - " fmt "\n", (SourceLoc)->Filename, (SourceLoc)->Line, (SourceLoc)->Column, ##__VA_ARGS__);    \
    _UBSAN_BREAK();  \
} while(0)

#else

#include <ntdef.h>
#include <debug.h>

#ifdef _UBSAN_HACK_
extern ULONG (*FrLdrDbgPrint)(const char *Format, ...);
#define DbgPrint FrLdrDbgPrint
#endif

#define ubsan_fail(SourceLoc, error, fmt, ...)  \
do {                                            \
    DbgPrint("UB SANITIZER: " error " at %s:%d:%d - " fmt "\n", (SourceLoc)->Filename, (SourceLoc)->Line, (SourceLoc)->Column, ##__VA_ARGS__);  \
    _UBSAN_BREAK();   \
} while(0)
#endif /* _UBSAN_MSVCRT_ */

FORCEINLINE
uint8_t is_integer_type(const struct TypeDescriptor* type)
{
    return type->TypeKind == 0;
}

FORCEINLINE
uint8_t is_signed_integer_type(const struct TypeDescriptor* type)
{
    return is_integer_type(type) && (type->TypeInfo & 1);
}

FORCEINLINE
uint8_t is_unsigned_integer_type(const struct TypeDescriptor* type)
{
    return is_integer_type(type) && !(type->TypeInfo & 1);
}

FORCEINLINE
uint8_t get_integer_bit_width(const struct TypeDescriptor* type)
{
    if (!is_integer_type(type))
        return 0;
    return 1 << (type->TypeInfo >> 1);
}

FORCEINLINE
ULONGLONG get_positive_int_value(ULONG_PTR value, const struct TypeDescriptor* type)
{
    uint8_t bit_width = get_integer_bit_width(type);

    if (!is_integer_type(type))
        return 0;

    if (bit_width > (sizeof(ULONG_PTR) * 8))
    {
        if (bit_width > 64) return 0xFFFFFFFFFFFFFFFFULL;
        return *((ULONGLONG*)value);
    }

    return value;
}

FORCEINLINE
LONGLONG get_negative_int_value(LONG_PTR value, const struct TypeDescriptor* type)
{
    uint8_t bit_width = get_integer_bit_width(type);

    if (!is_integer_type(type))
        return 0;

    if (bit_width > (sizeof(LONG_PTR) * 8))
    {
        if (bit_width > 64) return 0xFFFFFFFFFFFFFFFFLL;
        return *((LONGLONG*)value);
    }
    else
    {
        uint8_t sign_extend = (sizeof(LONG_PTR) * 8) - bit_width;
        return ((LONGLONG)value) << sign_extend >> sign_extend;
    }
}

FORCEINLINE
uint8_t is_negative_value(ULONG_PTR value, const struct TypeDescriptor* type)
{
    return get_negative_int_value(value, type) < 0;
}


