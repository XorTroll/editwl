cmake_minimum_required(VERSION 3.14)

project(ROM VERSION $ENV{ETWL_VERSION} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Xml)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Xml)
find_library(LIBEDITWL_LIB NAMES editwl libeditwl HINTS ../../libeditwl/build)

set(LIBEDITWL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../libeditwl)
set(MODULES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_compile_definitions(ETWL_MAJOR=$ENV{ETWL_MAJOR})
add_compile_definitions(ETWL_MINOR=$ENV{ETWL_MINOR})
add_compile_definitions(ETWL_MICRO=$ENV{ETWL_MICRO})
add_compile_definitions(ETWL_BUGFIX=$ENV{ETWL_BUGFIX})
add_compile_definitions(ETWL_VERSION="$ENV{ETWL_VERSION}")

include_directories(include ${MODULES_ROOT}/include ${LIBEDITWL_ROOT}/include)

set(SOURCES
    source/mod.cpp
)

set(QOBJ_SOURCES
)

add_library(ROM SHARED
    ${SOURCES}
    ${QOBJ_SOURCES}
)

target_link_libraries(ROM PRIVATE ${LIBEDITWL_LIB})
target_link_libraries(ROM PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

add_custom_command(TARGET ROM POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:ROM>
                    $ENV{ETWL_OUTPUT_MODULES_PATH}
                    COMMENT "Copying editwl-bin shared library module to output directory...")
