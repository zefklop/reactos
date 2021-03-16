
PROJECT(SHELL)

list(APPEND SOURCE
    CBandSite.cpp
    CBandSiteMenu.cpp
    CBaseBar.cpp
    CISFBand.cpp
    CSHEnumClassesOfCategories.cpp
    shellbars.h)

add_library(shellbars OBJECT ${SOURCE})
target_link_libraries(shellbars
    PRIVATE atl_classes
    PUBLIC libmsvcrt)
target_compile_definitions(shellbars PRIVATE UNICODE _UNICODE)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(shellbars PRIVATE -Wno-unused-but-set-variable)
endif()

add_pch(shellbars shellbars.h SOURCE)
add_dependencies(shellbars xdk psdk)
