
# do not try to sanitize the sanitizer.
add_compile_options(-fno-sanitize=all)

# Disabling breaks for now
add_compile_definitions(_UBSAN_NOBREAK_)

list(APPEND SOURCE
    add_overflow.c
    cplusplus.c
    builtin_unreachable.c
    divrem_overflow.c
    invalid_builtin.c
    load_invalid_value.c
    mul_overflow.c
    negate_overflow.c
    nonnull_arg.c
    out_of_bounds.c
    pointer_overflow.c
    shift_out_of_bounds.c
    sub_overflow.c
    type_mismatch_v1.c)

add_library(gcc_ubsan_nt STATIC ${SOURCE})

add_library(gcc_ubsan_hack STATIC ${SOURCE})
target_compile_definitions(gcc_ubsan_hack PRIVATE _UBSAN_HACK_)

add_library(gcc_ubsan_nobreak STATIC ${SOURCE})
target_compile_definitions(gcc_ubsan_nobreak PRIVATE _UBSAN_NOBREAK_)

add_library(gcc_ubsan_win32k STATIC ${SOURCE})
target_compile_definitions(gcc_ubsan_win32k PRIVATE _UBSAN_WIN32K_)

add_library(gcc_ubsan_videoprt STATIC ${SOURCE})
target_compile_definitions(gcc_ubsan_videoprt PRIVATE _UBSAN_VIDEOPRT_)

add_library(gcc_ubsan_scsiport STATIC ${SOURCE})
target_compile_definitions(gcc_ubsan_scsiport PRIVATE _UBSAN_SCSIPORT_)

add_library(gcc_ubsan_msvcrt STATIC ${SOURCE})
target_compile_definitions(gcc_ubsan_msvcrt PRIVATE _UBSAN_MSVCRT_)
target_link_libraries(gcc_ubsan_msvcrt PRIVATE libkernel32 libmsvcrt_real)
