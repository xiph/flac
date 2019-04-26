set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "${CMAKE_INSTALL_PREFIX}")
set(libdir "${CMAKE_INSTALL_FULL_LIBDIR}")
set(includedir "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
configure_file(flac++.pc.in flac++.pc @ONLY)

check_cxx_source_compiles("
    #ifdef __STDC_NO_VLA__
    syntax error;
    #else
    int fvla (int m, int * c)
    {
      int D[m];
      return D[0] == c[0];
    }

    int main(int, char * []) { return 0; }
    #endif"
    HAVE_CXX_VARARRAYS)

set(SOURCES
    metadata.cpp
    stream_decoder.cpp
    stream_encoder.cpp)

add_library(FLAC++-static STATIC ${SOURCES})
target_compile_definitions(FLAC++-static PUBLIC FLAC__NO_DLL)
target_include_directories(FLAC++-static INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>")
target_link_libraries(FLAC++-static PUBLIC FLAC-static)

add_library(FLAC++ SHARED ${SOURCES})
target_compile_definitions(FLAC++ PRIVATE FLACPP_API_EXPORTS FLAC__USE_VISIBILITY_ATTR)
target_include_directories(FLAC++ INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>")
target_link_libraries(FLAC++ PRIVATE FLAC)
set_target_properties(FLAC++ PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VERSION 6.3.0
    SOVERSION 6)

install(TARGETS FLAC++ FLAC++-static EXPORT targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/flac++.pc"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig")
