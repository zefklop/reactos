
include_directories(include)

add_definitions(-D_CRTBLD)

if(ARCH STREQUAL "i386")
    list(APPEND CHKSTK_ASM_SOURCE except/i386/chkstk_asm.s)
    if(NOT MSVC)
        list(APPEND CHKSTK_SOURCE except/i386/chkstk_ms.s)
    endif()
elseif(ARCH STREQUAL "amd64")
    list(APPEND CHKSTK_ASM_SOURCE except/amd64/chkstk_ms.s)
elseif(ARCH STREQUAL "arm")
    list(APPEND CHKSTK_ASM_SOURCE except/arm/chkstk_asm.s)
    if(NOT MSVC)
        list(APPEND CHKSTK_SOURCE except/arm/chkstk_ms.s)
    endif()
elseif(ARCH STREQUAL "powerpc")
    list(APPEND CHKSTK_ASM_SOURCE except/powerpc/chkstk_asm.s)
endif()

add_asm_files(chkstk_lib_asm ${CHKSTK_ASM_SOURCE})
add_library(chkstk ${CHKSTK_SOURCE} ${chkstk_lib_asm})
set_target_properties(chkstk PROPERTIES LINKER_LANGUAGE "C")
add_dependencies(chkstk asm)

include(crt.cmake)
include(libcntpr.cmake)
include(msvcrtex.cmake)
include(oldnames.cmake)

add_library(user32_wsprintf
    printf/streamout.c
    printf/wstreamout.c
    printf/wsprintfA.c
    printf/wsprintfW.c
    printf/wvsprintfA.c
    printf/wvsprintfW.c
    printf/wvsnprintfA.c
    printf/wvsnprintfW.c
    string/strnlen.c
    string/wcsnlen.c
    string/mbstowcs_nt.c
    string/wcstombs_nt.c)

add_dependencies(user32_wsprintf psdk)
target_compile_definitions(user32_wsprintf PRIVATE _USER32_WSPRINTF)

# For some reason, some CRT functions are only present in ntoskrnl for amd64
# We make them available for all of our archs by embedding a few of them in
# the ntoskrnl import lib, like we do for msvcrt
# cos, sin & atan shouldn't really be there, but we need it for win32k
# until someone rewrites arc functions without floating point
if (ARCH STREQUAL "i386")
    add_asm_files(NTOSKRNLEX_SRC
        math/i386/atan2_asm.s
        math/i386/cos_asm.s
        math/i386/sin_asm.s
        math/i386/tan_asm.s
        setjmp/i386/setjmp.s)
    list(APPEND NTOSKRNLEX_SRC mem/memcmp.c)
elseif(ARCH STREQUAL "amd64")
    add_asm_files(NTOSKRNLEX_SRC
        math/amd64/atan2.S
        math/amd64/tan.S)
    list(APPEND NTOSKRNLEX_SRC
        math/cos.c
        math/sin.c)
else()
    list(APPEND NTOSKRNLEX_SRC mem/memcmp.c)
endif()
add_library(ntoskrnlex OBJECT ${NTOSKRNLEX_SRC})
# Use the same definitions as libcntpr
target_compile_definitions(ntoskrnlex PRIVATE $<TARGET_PROPERTY:libcntpr,COMPILE_DEFINITIONS>)
add_dependencies(ntoskrnlex psdk asm)

add_library(getopt misc/getopt.c)
target_compile_definitions(getopt PRIVATE _DLL __USE_CRTIMP)
add_dependencies(getopt psdk)

add_library(strtol
    string/ctype.c
    string/iswctype.c
    string/strtoi64.c
    string/strtol.c
    string/strtoul.c
    string/strtoull.c
    string/wctype.c)
target_compile_definitions(strtol PRIVATE _LIBCNT_)
add_dependencies(strtol psdk)


if(ARCH STREQUAL "i386")
    list(APPEND ATAN2_ASM_SOURCE math/i386/atan2_asm.s)
elseif(ARCH STREQUAL "amd64")
    list(APPEND ATAN2_ASM_SOURCE math/amd64/atan2.S)
elseif(ARCH STREQUAL "arm")
    list(APPEND ATAN2_ASM_SOURCE math/arm/atan2.s)
endif()

add_asm_files(atan2_asm ${ATAN2_ASM_SOURCE})
add_library(atan2 ${atan2_asm})
set_target_properties(atan2 PROPERTIES LINKER_LANGUAGE "C")
add_dependencies(atan2 asm)


