
add_definitions(
    -D_NTSYSTEM_
    -DNASSERT)

list(APPEND SOURCE
    cminit.c
    cmfindsubkey.c
    compressedname.c
    cmindex.c
    cmkeydel.c
    cmname.c
    cmprepare.c
    cmse.c
    cmvalue.c
    compareindex.c
    comparename.c
    create.c
    free.c
    freecell.c
    flathive.c
    hashkey.c
    hivebin.c
    hivecell.c
    hivefree.c
    hiveload.c
    hivesum.c
    hivewrt.c
    HvGetCell.c
    HvInitialize.c
    memhive.c
    trace.c
    valuename.c
    verify.c
    cmlib.h
    valuedata.c)

if(CMAKE_CROSSCOMPILING)
    add_library(cmlib ${SOURCE})
    add_dependencies(cmlib bugcodes xdk)
    add_pch(cmlib cmlib.h SOURCE)
else()
    add_definitions(
        -D__NO_CTYPE_INLINES
        -DCMLIB_HOST)
    add_library(cmlibhost ${SOURCE})
    target_include_directories(cmlibhost INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

    if(NOT MSVC)
        target_compile_options(cmlibhost PRIVATE -fshort-wchar -Wno-multichar)
    endif()

    target_link_libraries(cmlibhost PRIVATE host_includes)
endif()
