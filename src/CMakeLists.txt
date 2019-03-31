cmake_minimum_required(VERSION 3.12)

option(ENABLE_64_BIT_WORDS "Set FLAC__BYTES_PER_WORD to 8 (4 is the default)" OFF)
option(WITH_OGG "ogg support (default: test for libogg)" ON)
option(WITH_XMMS "Build XMMS plugin" OFF)

if(WITH_OGG)
    find_package(OGG REQUIRED)
endif()

check_include_file("iconv.h" HAVE_ICONV_H)

add_compile_definitions(
    ENABLE_64_BIT_WORDS=$<BOOL:${ENABLE_64_BIT_WORDS}>
    FLAC__HAS_OGG=$<BOOL:${OGG_FOUND}>
    WORDS_BIGENDIAN=$<BOOL:${CPU_IS_BIG_ENDIAN}>
    $<$<BOOL:${HAVE_ICONV_H}>:HAVE_ICONV>
    $<$<BOOL:${HAVE_SYS_PARAM_H}>:HAVE_SYS_PARAM_H>)

add_subdirectory("libFLAC")
if(BUILD_CXXLIBS)
    add_subdirectory("libFLAC++")
endif()
add_subdirectory("flac")
add_subdirectory("metaflac")
add_subdirectory("share")
add_subdirectory("utils")

if(WITH_XMMS)
    add_subdirectory("plugin_common")
    add_subdirectory("plugin_xmms")
endif()

if(BUILD_TESTING)
    add_subdirectory("test_libs_common")
    add_subdirectory("test_libFLAC")
    if(BUILD_CXXLIBS)
        add_subdirectory("test_libFLAC++")
    endif()
    add_subdirectory("test_grabbag")
    add_subdirectory("test_seeking")
    add_subdirectory("test_streams")
endif()
