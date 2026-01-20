cmake_minimum_required(VERSION 3.16)

option(EKG_STATIC_BUILD "Build fully statically linked executable on Windows (static CRT; requires static Qt)" OFF)

if(EKG_STATIC_BUILD AND WIN32)
  cmake_policy(SET CMP0091 NEW)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(ekg VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Quick Charts)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Quick Charts)

file(GLOB_RECURSE PROJECT_SOURCES
        ${PROJECT_SOURCE_DIR}/src/*.cpp
        ${PROJECT_SOURCE_DIR}/src/*.h
        ${PROJECT_SOURCE_DIR}/include/*.h
        ${PROJECT_SOURCE_DIR}/main.cpp
)

set(MACOSX_BUNDLE_ICON_FILE app.icns)

set(app_icon_macos "${CMAKE_CURRENT_SOURCE_DIR}/app.icns")
set_source_files_properties(${app_icon_macos} PROPERTIES
       MACOSX_PACKAGE_LOCATION "Resources")

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/app.ico")

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ekg
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        qml.qrc
        ${ICON_FILE}
        ${app_icon_macos}
        ${app_icon_resource_windows}
    )
else()
    if(ANDROID)
        add_library(ekg SHARED
            ${PROJECT_SOURCES}
            qml.qrc
        )
    else()
        add_executable(ekg
            ${PROJECT_SOURCES}
            qml.qrc
            ${ICON_FILE}
            ${app_icon_macos}
            ${app_icon_windows}
        )
    endif()
endif()

target_link_libraries(ekg PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Charts
)

if(EKG_STATIC_BUILD AND WIN32)
  if(MSVC)
    set_property(TARGET ekg PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  elseif(MINGW)
    target_link_options(ekg PRIVATE -static -static-libgcc -static-libstdc++)
  endif()
  if(QT_VERSION_MAJOR EQUAL 6)
    qt_import_plugins(ekg INCLUDE Qt${QT_VERSION_MAJOR}::QWindowsIntegrationPlugin)
  endif()
endif()

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ekg)
endif()
set_target_properties(ekg PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ekg
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ekg)
endif()
