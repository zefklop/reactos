project(SHELL)

list(APPEND SOURCE
    CChangeNotifyServer.cpp
    CDesktopBrowser.cpp
    CDirectoryWatcher.cpp
    CDirectoryList.cpp
    dde.cpp)

add_library(shelldesktop OBJECT ${SOURCE})
target_link_libraries(shelldesktop
    PRIVATE atl_classes
    PUBLIC libmsvcrt)
target_compile_definitions(shelldesktop PRIVATE UNICODE _UNICODE)

add_dependencies(shelldesktop xdk psdk)
