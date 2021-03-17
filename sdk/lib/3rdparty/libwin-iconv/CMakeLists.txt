
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_library(iconv-static win_iconv.c)
set_target_properties(iconv-static PROPERTIES OUTPUT_NAME "iconv")
target_link_libraries(iconv-static PUBLIC libmsvcrt)
add_dependencies(iconv-static psdk)
