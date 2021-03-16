project(SHELL)

list(APPEND SOURCE
    recyclebin.c
    recyclebin_generic.c
    recyclebin_generic_enumerator.c
    recyclebin_v5.c
    recyclebin_v5_enumerator.c
    recyclebin_private.h)

add_library(recyclebin ${SOURCE} guid.c)
target_compile_definitions(recyclebin PRIVATE UNICODE _UNICODE)
target_link_libraries(recyclebin PUBLIC libmsvcrt)
add_pch(recyclebin recyclebin_private.h SOURCE)
add_dependencies(recyclebin psdk)
