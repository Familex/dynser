#######################################
# Source
#######################################

add_library (
    dynser
    "dynser.h"

    "dynser/dynser.h" "dynser/dynser.cpp"

    "structs/fields.hpp"
    "structs/properties.h" "structs/properties.cpp"
    "structs/context.hpp"

    "config/config.h" "config/config.cpp"
    "regex/structures.h" "regex/structures.cpp"
    "regex/to_string.h" "regex/to_string.cpp"
    "regex/from_string.h" "regex/from_string.cpp"
    "config/structures.h" "config/structures.cpp"
    "config/keywords.h"

    "util/mapper_helpers.hpp"
    "util/prefix.hpp"
    "util/visit.hpp"
)

set_property (TARGET dynser PROPERTY CXX_STANDARD 23)

include_directories (
    dynser
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

#######################################
# External projects
#######################################

include (../download_project/DownloadProject.cmake)

# lua_static
download_project (
    PROJ lua_cmake
    GIT_REPOSITORY https://github.com/walterschell/Lua.git
    GIT_TAG master
    UPDATE_DISCONNECTED 1
)
add_subdirectory (${lua_cmake_SOURCE_DIR} ${lua_cmake_BINARY_DIR})

# luwra (header-only)
download_project (
    PROJ luwra
    GIT_REPOSITORY https://github.com/vapourismo/luwra.git
    GIT_TAG master
    UPDATE_DISCONNECTED 1
)
include_directories ("${luwra_SOURCE_DIR}/lib")

# yaml-cpp
download_project (
    PROJ yaml_cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG master
    UPDATE_DISCONNECTED 1
)
add_subdirectory (${yaml_cpp_SOURCE_DIR} ${yaml_cpp_BINARY_DIR})
include_directories ("${yaml_cpp_SOURCE_DIR}/include")

#######################################
# Flags
#######################################

# from https://stackoverflow.com/a/60890947
# to define __cplusplus correctly for MSVC
if ((MSVC) AND (MSVC_VERSION GREATER_EQUAL 1914))
    target_compile_options (dynser PUBLIC "/Zc:__cplusplus") # FIXME can be INTERFACE?
endif()

#######################################
# Link
#######################################

target_link_libraries (
    dynser
    lua_static
    yaml-cpp
)

#######################################
# Paths
#######################################

target_include_directories (
    dynser 
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${luwra_SOURCE_DIR}/lib"
)
