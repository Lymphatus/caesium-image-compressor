cmake_minimum_required(VERSION 3.5)

project(caesium_image_compressor LANGUAGES CXX)

if (CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif ()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS views)

if (APPLE)
    set(MACOSX_DEPLOYMENT_TARGET 10.14)
    set(MACOSX_BUNDLE_ICON_FILE icon.icns)
    set(CAESIUM_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/icon.icns)
    set_source_files_properties(${CAESIUM_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
elseif(WIN32)
    set(CAESIUM_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons.rc)
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nullability-completeness")

if (NOT (DEFINED ENV{Qt5_DIR} OR DEFINED Qt5_DIR))
    message(FATAL_ERROR "Qt5_DIR is not defined. Cannot find Qt5 dir.")
endif ()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/include)

if (APPLE)
    link_directories(/usr/local/lib)
elseif (WIN32)
    link_directories("C:\\libcaesium\\lib")
endif ()

find_package(Qt5 COMPONENTS Widgets Core Gui Concurrent Svg REQUIRED)

set(SOURCES
        src/main.cpp
        src/MainWindow.cpp
        src/models/CImage.cpp
        src/models/CImageTreeItem.cpp
        src/models/CImageTreeModel.cpp
        src/utils/Utils.cpp
        src/dialogs/AboutDialog.cpp
        src/widgets/QDropTreeView.cpp
        src/widgets/QZoomGraphicsView.cpp
        src/delegates/HtmlDelegate.cpp
        src/exceptions/ImageNotSupportedException.cpp)

set(HEADERS
        src/vendor/caesium.h
        src/MainWindow.h
        src/models/CImage.h
        src/models/CImageTreeItem.h
        src/models/CImageTreeModel.h
        src/utils/Utils.h
        src/dialogs/AboutDialog.h
        src/widgets/QDropTreeView.h
        src/widgets/QZoomGraphicsView.h
        src/delegates/HtmlDelegate.h
        src/exceptions/ImageNotSupportedException.h)

set(UI
        views/MainWindow.ui
        views/AboutDialog.ui)

set(RESOURCES resources/resources.qrc)

add_executable(caesium_image_compressor MACOSX_BUNDLE ${SOURCES} ${HEADERS} ${UI} ${RESOURCES} ${CAESIUM_ICON})

if (APPLE)
    set_target_properties(caesium_image_compressor PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist")
endif ()

set_target_properties(caesium_image_compressor PROPERTIES OUTPUT_NAME "Caesium Image Compressor")
target_link_libraries(caesium_image_compressor PRIVATE Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Concurrent Qt5::Svg caesium)
