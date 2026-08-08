cmake_minimum_required(VERSION 3.14)

project(editwl VERSION $ENV{ETWL_VERSION} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Xml)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Xml)
find_library(LIBEDITWL_LIB NAMES editwl libeditwl HINTS ../libeditwl/build)

set(LIBEDITWL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../libeditwl)

add_compile_definitions(ETWL_MAJOR=$ENV{ETWL_MAJOR})
add_compile_definitions(ETWL_MINOR=$ENV{ETWL_MINOR})
add_compile_definitions(ETWL_MICRO=$ENV{ETWL_MICRO})
add_compile_definitions(ETWL_BUGFIX=$ENV{ETWL_BUGFIX})
add_compile_definitions(ETWL_MODULES_DIR="$ENV{ETWL_MODULES_DIR}")
add_compile_definitions(ETWL_VERSION="$ENV{ETWL_VERSION}")

include_directories(include ${LIBEDITWL_ROOT}/include)

set(SOURCES
    source/main.cpp
    source/base_Include.cpp
    source/ui/ui_MainWindow.cpp
    source/ui/ui_SubWindow.cpp

    source/bmg/bmg_CliHandler.cpp
    source/bmg/bmg_UiHandler.cpp
    source/bmg/ui/ui_BmgSubWindow.cpp
    source/bmg/ui/ui_BmgMessageSubWindow.cpp
    source/bmg/bmg.cpp

    source/rom/rom_CliHandler.cpp
    source/rom/rom_UiHandler.cpp
)

set(QOBJ_SOURCES
    include/ui/ui_MainWindow.hpp source/ui/ui_MainWindow.ui

    include/bmg/ui/ui_BmgSubWindow.hpp source/bmg/ui/ui_BmgSubWindow.ui
    include/bmg/ui/ui_BmgMessageSubWindow.hpp source/bmg/ui/ui_BmgMessageSubWindow.ui
)

set(PROJECT_SOURCES
    ${SOURCES}
    ${QOBJ_SOURCES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(editwl
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(editwl
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(editwl PRIVATE ${LIBEDITWL_LIB})
target_link_libraries(editwl PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

add_custom_command(TARGET editwl POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:editwl>
                    $ENV{ETWL_OUTPUT_PATH}
                    COMMENT "Copying executable to output directory...")

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.editwl)
endif()
set_target_properties(editwl PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS editwl
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(editwl)
endif()
