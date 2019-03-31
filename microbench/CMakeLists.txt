if(MSVC)
    return()
endif()

set(CMAKE_REQUIRED_LIBRARIES rt)
check_function_exists(clock_gettime HAVE_CLOCK_GETTIME)

add_compile_definitions(
    $<$<BOOL:${HAVE_CLOCK_GETTIME}>:HAVE_CLOCK_GETTIME>
    $<$<BOOL:${APPLE}>:FLAC__SYS_DARWIN>)

add_executable(benchmark_residual benchmark_residual.c util.c)
target_include_directories(benchmark_residual PRIVATE
    "$<TARGET_PROPERTY:FLAC-static,SOURCE_DIR>/include")
target_link_libraries(benchmark_residual
    FLAC-static
    $<$<BOOL:${HAVE_CLOCK_GETTIME}>:rt>)
