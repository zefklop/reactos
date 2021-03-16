
list(APPEND SOURCE
    shimlib.c
    shimlib.h
    # These .inl functions are included so they show up in vs
    setup_shim.inl
    implement_shim.inl)

add_library(shimlib ${SOURCE})
target_link_libraries(shimlib PUBLIC ntdll_crt)
add_dependencies(shimlib xdk)
