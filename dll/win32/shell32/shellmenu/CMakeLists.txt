project(SHELL)

list(APPEND SOURCE
    CMenuBand.cpp
    CMenuDeskBar.cpp
    CMenuFocusManager.cpp
    CMenuSite.cpp
    CMenuToolbars.cpp
    CMergedFolder.cpp
    CStartMenu.cpp)

add_library(shellmenu OBJECT ${SOURCE})
target_link_libraries(shellmenu
    PRIVATE atl_classes
    PUBLIC libmsvcrt)
target_compile_definitions(shellmenu PRIVATE UNICODE _UNICODE)
add_dependencies(shellmenu xdk psdk)
