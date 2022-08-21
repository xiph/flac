cmake_minimum_required(VERSION 3.9)

find_package(Doxygen)

if (NOT DOXYGEN_FOUND)
    return()
endif()

option(BUILD_DOXYGEN "Enable API documentation building via Doxygen" ON)

if (NOT BUILD_DOXYGEN)
    return()
endif()

set(DOXYGEN_HTML_FOOTER doxygen.footer.html)
set(DOXYGEN_GENERATE_TAGFILE FLAC.tag)
set(DOXYGEN_HTML_OUTPUT api)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.12)
    doxygen_add_docs(FLAC-doxygen
        ALL
        "${PROJECT_SOURCE_DIR}/include/FLAC"
        "${PROJECT_SOURCE_DIR}/include/FLAC++")
else()
    doxygen_add_docs(FLAC-doxygen
        "${PROJECT_SOURCE_DIR}/include/FLAC"
        "${PROJECT_SOURCE_DIR}/include/FLAC++")
endif()

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/api/"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}/api")
