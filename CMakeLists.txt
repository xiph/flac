cmake_minimum_required(VERSION 3.1)

if(NOT (CMAKE_BUILD_TYPE OR CMAKE_CONFIGURATION_TYPES OR DEFINED ENV{CFLAGS} OR DEFINED ENV{CXXFLAGS}))
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo")
endif()

project(FLAC VERSION 1.3.2) # HOMEPAGE_URL "https://www.xiph.org/flac/")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(BUILD_CXXLIBS "Build libFLAC++" ON)
option(BUILD_EXAMPLES "Build and install examples" ON)
option(WITH_OGG "ogg support (default: test for libogg)" ON)

if(WITH_OGG)
    find_package(OGG REQUIRED)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wstrict-prototypes -Wmissing-prototypes -Waggregate-return -Wcast-align -Wnested-externs -Wshadow -Wundef -Wmissing-declarations -Winline")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -funroll-loops")

    option(ENABLE_SSP "Enable GNU GCC stack smash protection" OFF)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wcast-align -Wshadow -Wwrite-strings -Wctor-dtor-privacy -Wnon-virtual-dtor -Wreorder -Wsign-promo -Wundef")
endif()
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    set(CMAKE_EXE_LINKER_FLAGS -no-pie)
endif()

include(CMakePackageConfigHelpers)
include(CPack)
include(CTest)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)
include(GNUInstallDirs)
include(UseSystemExtensions)
include(TestBigEndian)

check_include_file("byteswap.h" HAVE_BYTESWAP_H)
check_include_file("inttypes.h" HAVE_INTTYPES_H)
check_include_file("stdint.h" HAVE_STDINT_H)
check_include_file("x86intrin.h" FLAC__HAS_X86INTRIN)

check_function_exists(fseeko HAVE_FSEEKO)

check_c_source_compiles("int main() { return __builtin_bswap16 (0) ; }" HAVE_BSWAP16)
check_c_source_compiles("int main() { return __builtin_bswap32 (0) ; }" HAVE_BSWAP32)

test_big_endian(CPU_IS_BIG_ENDIAN)

check_c_compiler_flag(-Werror HAVE_WERROR_FLAG)
check_c_compiler_flag(-Wdeclaration-after-statement HAVE_DECL_AFTER_STMT_FLAG)
check_c_compiler_flag("-fstack-protector --param ssp-buffer-size=4" HAVE_SSP_FLAG)
check_c_compiler_flag(-mstackrealign HAVE_STACKREALIGN_FLAG)
check_cxx_compiler_flag(-Weffc++ HAVE_WEFFCXX_FLAG)

if(HAVE_WERROR_FLAG)
    option(ENABLE_WERROR "Enable -Werror in all Makefiles" OFF)
endif()

add_compile_options(
    $<$<BOOL:${MSVC}>:/wd4267>
    $<$<BOOL:${MSVC}>:/wd4996>
    $<$<BOOL:${ENABLE_WERROR}>:-Werror>
    $<$<AND:$<BOOL:${HAVE_SSP_FLAG}>,$<BOOL:${ENABLE_SSP}>>:-fstack-protector>
    $<$<AND:$<BOOL:${HAVE_SSP_FLAG}>,$<BOOL:${ENABLE_SSP}>>:--param>
    $<$<AND:$<BOOL:${HAVE_SSP_FLAG}>,$<BOOL:${ENABLE_SSP}>>:ssp-buffer-size=4>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<BOOL:${HAVE_WEFFCXX_FLAG}>>:-Weffc++>
    $<$<AND:$<COMPILE_LANGUAGE:C>,$<BOOL:${HAVE_DECL_AFTER_STMT_FLAG}>>:-Wdeclaration-after-statement>)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "i686" AND HAVE_STACKREALIGN_FLAG)
    add_compile_options(-mstackrealign)
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

add_subdirectory("doc")
add_subdirectory("src")
add_subdirectory("microbench")
if(BUILD_EXAMPLES)
    add_subdirectory("examples")
endif()
if(BUILD_TESTING)
    add_subdirectory("test")
endif()

configure_file(config.cmake.h.in config.h)

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
        "cmake/FindOGG.cmake"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cmake")

file(GLOB FLAC_HEADERS "include/FLAC/*.h")
file(GLOB FLAC++_HEADERS "include/FLAC++/*.h")
install(FILES ${FLAC_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/FLAC")
install(FILES ${FLAC++_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/FLAC++")
install(FILES "man/flac.1" "man/metaflac.1" DESTINATION "${CMAKE_INSTALL_MANDIR}")
