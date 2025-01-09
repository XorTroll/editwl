cmake_minimum_required(VERSION 3.14)

project(editwl-bin VERSION $ENV{ETWL_VERSION} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_library(LIBEDITWL_LIB NAMES editwl libeditwl HINTS ../libeditwl/build)

set(LIBEDITWL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../libeditwl)
set(MODULES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../modules)

add_compile_definitions(ETWL_MAJOR=$ENV{ETWL_MAJOR})
add_compile_definitions(ETWL_MINOR=$ENV{ETWL_MINOR})
add_compile_definitions(ETWL_MICRO=$ENV{ETWL_MICRO})
add_compile_definitions(ETWL_BUGFIX=$ENV{ETWL_BUGFIX})
add_compile_definitions(ETWL_MODULES_DIR="$ENV{ETWL_MODULES_DIR}")
add_compile_definitions(ETWL_VERSION="$ENV{ETWL_VERSION}")

include_directories(include ${MODULES_ROOT}/include ${LIBEDITWL_ROOT}/include)

set(SOURCES
    source/main.cpp
    source/base_Include.cpp
    source/mod/mod_Loader.cpp
    source/ui/ui_MainWindow.cpp
)

set(QOBJ_SOURCES
    include/ui/ui_MainWindow.hpp source/ui/ui_MainWindow.ui
)

set(PROJECT_SOURCES
    ${SOURCES}
    ${QOBJ_SOURCES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(editwl-bin
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET editwl-bin APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(editwl-bin SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(editwl-bin
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(editwl-bin PRIVATE ${LIBEDITWL_LIB})
target_link_libraries(editwl-bin PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

add_custom_command(TARGET editwl-bin POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:editwl-bin>
                    $ENV{ETWL_OUTPUT_PATH}
                    COMMENT "Copying executable to output directory...")

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.editwl-bin)
endif()
set_target_properties(editwl-bin PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS editwl-bin
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(editwl-bin)
endif()
