
list(APPEND SOURCE
    check/boot.c
    check/check.c
    check/common.c
    check/fat.c
    check/file.c
    check/io.c
    check/lfn.c
    common.c
    fat12.c
    fat16.c
    fat32.c
    vfatlib.c
    vfatlib.h)

add_library(vfatlib ${SOURCE})
target_link_libraries(vfatlib PUBLIC libmsvcrt)
add_dependencies(vfatlib xdk)
add_pch(vfatlib vfatlib.h SOURCE)

add_library(vfatlib_nt ${SOURCE})
target_link_libraries(vfatlib_nt PUBLIC ntdll_crt)
add_dependencies(vfatlib_nt xdk)
add_pch(vfatlib_nt vfatlib.h SOURCE)
