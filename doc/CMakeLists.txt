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

if(CMAKE_VERSION VERSION_LESS 3.12)
    doxygen_add_docs(FLAC-doxygen
        ALL
        "${PROJECT_SOURCE_DIR}/include/FLAC"
        "${PROJECT_SOURCE_DIR}/include/FLAC++")
else()
    doxygen_add_docs(FLAC-doxygen
        "${PROJECT_SOURCE_DIR}/include/FLAC"
        "${PROJECT_SOURCE_DIR}/include/FLAC++")

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/html/"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}/html/api")

endif()

install(FILES
    html/images/logo.svg
    html/images/logo130.gif
    html/changelog.html
    html/developers.html
    html/documentation.html
    html/documentation_bugs.html
    html/documentation_example_code.html
    html/documentation_format_overview.html
    html/documentation_tools.html
    html/documentation_tools_flac.html
    html/documentation_tools_metaflac.html
    html/faq.html
    html/favicon.ico
    html/features.html
    html/flac.css
    html/format.html
    html/id.html
    html/index.html
    html/license.html
    html/ogg_mapping.html
DESTINATION "${CMAKE_INSTALL_DOCDIR}/html")
