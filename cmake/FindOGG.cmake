find_package(PkgConfig)
pkg_check_modules(_OGG QUIET ogg)

find_path(_OGG_INCLUDE_DIR
    NAMES "ogg/ogg.h"
    PATHS ${_OGG_INCLUDE_DIRS})

find_library(_OGG_LIBRARY
    NAMES ogg libogg
    HINTS ${_OGG_LIBRARY_DIRS})

find_library(_OGG_LIBRARY_STATIC
    NAMES libogg.a libogg_static
    HINTS ${_OGG_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OGG
    REQUIRED_VARS _OGG_INCLUDE_DIR _OGG_LIBRARY
    VERSION_VAR _OGG_VERSION)

if(OGG_FOUND)
    if(NOT Ogg::Ogg)
        add_library(Ogg::Ogg UNKNOWN IMPORTED)
        set_target_properties(Ogg::Ogg PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_OGG_INCLUDE_DIR}"
            IMPORTED_LOCATION "${_OGG_LIBRARY}")
    endif()
    if(NOT Ogg::Ogg-static AND _OGG_LIBRARY_STATIC)
        add_library(Ogg::Ogg-static STATIC IMPORTED)
        set_target_properties(Ogg::Ogg-static PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_OGG_INCLUDE_DIR}"
            IMPORTED_LOCATION "${_OGG_LIBRARY_STATIC}")
    endif()
endif()
