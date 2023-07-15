cmake_minimum_required(VERSION 3.5)

project(draw VERSION 1.0 LANGUAGES CXX)
ADD_DEFINITIONS(-DQT_NO_KEYWORDS)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)
find_package(OpenSSL REQUIRED)

if(OPENSSL_FOUND)
  include_directories(${OPENSSL_INCLUDE_DIRS})
  message(STATUS "OpenSSL Found!")
endif()

include_directories("sioclient/include")
link_directories("sioclient/lib")
include_directories("pointSoftenExternal/include")
include_directories("json/include")
link_directories("pointSoftenExternal/lib")
set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        painter.cpp
        curve.cpp
        bmp.cpp
        color.cpp
        mainwindow.h
        painter.h
        curve.h
        bmp.h
        color.h
        mainwindow.ui
        icon.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(draw
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET test APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(draw SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(draw
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(draw PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    libsioclient.a
    pointSoftenExternal.a
    -lWs2_32
    -fopenmp
)



set_target_properties(draw PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS draw
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(draw)
endif()
