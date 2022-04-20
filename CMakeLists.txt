# 3.1 is OK for most parts. However:
# 3.3 is needed in src/libFLAC
# 3.5 is needed in src/libFLAC/ia32
# 3.9 is needed in 'doc' because of doxygen_add_docs()
cmake_minimum_required(VERSION 3.5)

if(NOT (CMAKE_BUILD_TYPE OR CMAKE_CONFIGURATION_TYPES OR DEFINED ENV{CFLAGS} OR DEFINED ENV{CXXFLAGS}))
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo")
endif()

project(FLAC VERSION 1.3.4) # HOMEPAGE_URL "https://www.xiph.org/flac/")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(BUILD_CXXLIBS "Build libFLAC++" ON)
option(BUILD_PROGRAMS "Build and install programs" ON)
option(BUILD_EXAMPLES "Build and install examples" ON)
option(BUILD_TESTING "Build tests" ON)
option(BUILD_DOCS "Build and install doxygen documents" ON)
option(WITH_FORTIFY_SOURCE "Enable protection against buffer overflows" ON)
option(WITH_STACK_PROTECTOR "Enable GNU GCC stack smash protection" ON)
option(INSTALL_MANPAGES "Install MAN pages" ON)
option(INSTALL_PKGCONFIG_MODULES "Install PkgConfig modules" ON)
option(INSTALL_CMAKE_CONFIG_MODULE "Install CMake package-config module" ON)
option(WITH_OGG "ogg support (default: test for libogg)" ON)
option(BUILD_SHARED_LIBS "Build shared instead of static libraries" OFF)

set(VERSION ${PROJECT_VERSION})

if(NOT UNIX)
    # This is to make sure testing works when building with a DLL
    set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/objs)
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/objs)
endif()

if(WITH_OGG)
    find_package(Ogg REQUIRED)
    set(OGG_PACKAGE "ogg")
endif()

find_program (HAVE_GIT git)

if(HAVE_GIT)
    execute_process(
        COMMAND git describe --tags --exact-match
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        )
    execute_process(
        COMMAND git log -1 --pretty=format:%h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        )
    execute_process(
        COMMAND git log -1 --pretty=format:%cd --date=format:%Y%m%d
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        )
endif()

find_package(Iconv)
set(HAVE_ICONV ${Iconv_FOUND})

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wstrict-prototypes -Wmissing-prototypes -Waggregate-return -Wcast-align -Wnested-externs -Wshadow -Wundef -Wmissing-declarations -Winline")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -funroll-loops")
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wcast-align -Wshadow -Wwrite-strings -Wctor-dtor-privacy -Wnon-virtual-dtor -Wreorder -Wsign-promo -Wundef")
endif()
if(MSVC)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /Ob2 /Oi /Ot /Oy")
endif()

include(CMakePackageConfigHelpers)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)
include(CheckLibraryExists)
include(GNUInstallDirs)
include(UseSystemExtensions)
include(TestBigEndian)
enable_testing()

check_include_file("byteswap.h" HAVE_BYTESWAP_H)
check_include_file("inttypes.h" HAVE_INTTYPES_H)
check_include_file("stdint.h" HAVE_STDINT_H)
if(MSVC)
    check_include_file("intrin.h" FLAC__HAS_X86INTRIN)
else()
    check_include_file("x86intrin.h" FLAC__HAS_X86INTRIN)
endif()

check_function_exists(fseeko HAVE_FSEEKO)

check_c_source_compiles("int main() { return __builtin_bswap16 (0) ; }" HAVE_BSWAP16)
check_c_source_compiles("int main() { return __builtin_bswap32 (0) ; }" HAVE_BSWAP32)

test_big_endian(CPU_IS_BIG_ENDIAN)

check_c_compiler_flag(-Werror HAVE_WERROR_FLAG)
check_c_compiler_flag(-Wdeclaration-after-statement HAVE_DECL_AFTER_STMT_FLAG)
check_c_compiler_flag(-mstackrealign HAVE_STACKREALIGN_FLAG)
check_cxx_compiler_flag(-Weffc++ HAVE_WEFFCXX_FLAG)

if(MINGW AND (WITH_FORTIFY_SOURCE OR WITH_STACK_PROTECTOR))
  check_library_exists("ssp.a"  __stack_chk_fail "" HAVE_LIBSSP)
  if(NOT HAVE_LIBSSP)
    message(WARNING "Could not find libssp in MinGW, stack protection and/or FORTIFY_SOURCE are unavailable")
  else()
    link_libraries("ssp.a")
  endif()
elseif(NOT MSVC)
  set(HAVE_LIBSSP 1)
endif()

if(WITH_STACK_PROTECTOR)
  if(NOT MSVC)
    check_c_compiler_flag("-fstack-protector-strong" HAVE_STACK_PROTECTOR_FLAG)
  endif()
endif()

if(HAVE_WERROR_FLAG)
    option(ENABLE_WERROR "Enable -Werror in all Makefiles" OFF)
endif()

add_compile_options(
    $<$<BOOL:${MSVC}>:/wd4267>
    $<$<BOOL:${MSVC}>:/wd4996>
    $<$<BOOL:${ENABLE_WERROR}>:-Werror>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<BOOL:${HAVE_WEFFCXX_FLAG}>>:-Weffc++>
    $<$<AND:$<COMPILE_LANGUAGE:C>,$<BOOL:${HAVE_DECL_AFTER_STMT_FLAG}>>:-Wdeclaration-after-statement>)

if(HAVE_LIBSSP) # Implies WITH_FORTIFY_SOURCE
  add_definitions(-D_FORTIFY_SOURCE=2)
else(WITH_FORTIFY_SOURCE)
  add_definitions(-D_FORTIFY_SOURCE=1)
endif()

if(HAVE_STACK_PROTECTOR_FLAG AND HAVE_LIBSSP)
    add_compile_options($<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-fstack-protector-strong>)
endif()

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "i686" AND HAVE_STACKREALIGN_FLAG)
    add_compile_options($<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-mstackrealign>)
endif()

include_directories("include")

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
add_definitions(-DHAVE_CONFIG_H)

if(MSVC)
    add_definitions(
        -D_CRT_SECURE_NO_WARNINGS
        -D_USE_MATH_DEFINES)
endif()
if(CMAKE_BUILD_TYPE STREQUAL Debug OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
    add_definitions(-DFLAC__OVERFLOW_DETECT)
endif()

add_subdirectory("src")
add_subdirectory("microbench")
if(BUILD_DOCS)
    add_subdirectory("doc")
endif()
if(BUILD_EXAMPLES)
    add_subdirectory("examples")
endif()
if(BUILD_TESTING)
    add_subdirectory("test")
endif()

# The following folder layout is mostly for MSVC
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set_target_properties(FLAC grabbag getopt replaygain_analysis replaygain_synthesis utf8 PROPERTIES FOLDER Libraries)
if(BUILD_CXXLIBS)
    set_target_properties(FLAC++ PROPERTIES FOLDER Libraries)
endif()
if(BUILD_PROGRAMS)
    set_target_properties(flacapp metaflac PROPERTIES FOLDER Programs)
endif()
if(BUILD_TESTING)
    set_target_properties(test_libFLAC test_libs_common test_picture test_seeking test_streams test_cuesheet PROPERTIES FOLDER Tests)
	if(BUILD_CXXLIBS)
	    set_target_properties(test_libFLAC++ PROPERTIES FOLDER Tests)
	endif()
endif()
if(BUILD_EXAMPLES)
    set_target_properties(decode_file decode_file_cxx encode_file encode_file_cxx PROPERTIES FOLDER Examples)
endif()
if(BUILD_UTILS)
    set_target_properties(flacdiff flactimer PROPERTIES FOLDER Utils)
endif()

configure_file(config.cmake.h.in config.h)

if(INSTALL_CMAKE_CONFIG_MODULE)
    install(
        EXPORT targets
        DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cmake"
        NAMESPACE FLAC::)

    configure_package_config_file(
        flac-config.cmake.in flac-config.cmake
        INSTALL_DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cmake")
    write_basic_package_version_file(
        flac-config-version.cmake COMPATIBILITY AnyNewerVersion)

    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/flac-config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/flac-config-version.cmake"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cmake")
endif()

file(GLOB FLAC_HEADERS "include/FLAC/*.h")
file(GLOB FLAC++_HEADERS "include/FLAC++/*.h")
install(FILES ${FLAC_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/FLAC")
install(FILES ${FLAC++_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/FLAC++")
if(INSTALL_MANPAGES)
    install(FILES "man/flac.1" "man/metaflac.1" DESTINATION "${CMAKE_INSTALL_MANDIR}")
endif()
