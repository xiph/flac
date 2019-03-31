add_executable(flacdiff main.cpp)
target_link_libraries(flacdiff 
    FLACXX-static
    $<TARGET_NAME_IF_EXISTS:win_utf8_io>)