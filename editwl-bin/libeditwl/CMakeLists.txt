cmake_minimum_required(VERSION 3.14)

project(libeditwl LANGUAGES CXX)

# Project to build libeditwl as a shared library, to be used dynamically by the base PC program and the modules

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBEDITWL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../libeditwl)

include_directories(include ${LIBEDITWL_ROOT}/include)

set(SOURCES
    ${LIBEDITWL_ROOT}/source/twl/fmt/nfs/nfs_NitroFs.cpp

    ${LIBEDITWL_ROOT}/source/twl/fmt/fmt_BMG.cpp
    ${LIBEDITWL_ROOT}/source/twl/fmt/fmt_ROM.cpp

    ${LIBEDITWL_ROOT}/source/twl/fs/fs_File.cpp

    ${LIBEDITWL_ROOT}/source/twl/gfx/gfx_Conversion.cpp

    ${LIBEDITWL_ROOT}/source/twl/util/util_Compression.cpp
    ${LIBEDITWL_ROOT}/source/twl/util/util_String.cpp
)

add_library(libeditwl SHARED
    ${SOURCES}
)

set_target_properties(libeditwl PROPERTIES PREFIX "")

add_custom_command(TARGET libeditwl POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:libeditwl>
                    $ENV{ETWL_OUTPUT_PATH}
                    COMMENT "Copying shared library to output directory...")
