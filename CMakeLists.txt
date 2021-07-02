cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(caesium_image_compressor LANGUAGES CXX)

add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x050F00)

if (CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif ()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)



set(CMAKE_AUTOUIC_SEARCH_PATHS views)

if (APPLE)
    set(MACOSX_DEPLOYMENT_TARGET 10.14)
    set(MACOSX_BUNDLE_ICON_FILE icon.icns)
    set(CAESIUM_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/icon.icns)
    set_source_files_properties(${CAESIUM_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
elseif(WIN32)
    set(CAESIUM_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons.rc)
endif ()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wno-deprecated-register")

if (NOT (DEFINED ENV{CMAKE_PREFIX_PATH} OR DEFINED CMAKE_PREFIX_PATH))
    message(FATAL_ERROR "CMAKE_PREFIX_PATH is not defined. Cannot find Qt5 dir.")
endif ()

if (APPLE)
    link_directories(/usr/local/lib)
elseif (WIN32)
    link_directories("C:\\libcaesium\\lib")
endif ()

find_package(Qt5 COMPONENTS Widgets Core Gui Concurrent Svg Network REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/zip)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/exiv2)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/include)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/zip/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/exiv2/include)



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
        src/exceptions/ImageNotSupportedException.cpp
        src/utils/Updater.cpp)

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
        src/exceptions/ImageNotSupportedException.h
        src/utils/Updater.h)

set(UI
        views/MainWindow.ui
        views/AboutDialog.ui)

set(RESOURCES resources/resources.qrc)

if (WIN32)
    add_executable(caesium_image_compressor WIN32 ${SOURCES} ${HEADERS} ${UI} ${RESOURCES} ${CAESIUM_ICON})
else()
    add_executable(caesium_image_compressor MACOSX_BUNDLE ${SOURCES} ${HEADERS} ${UI} ${RESOURCES} ${CAESIUM_ICON})
endif()
if (APPLE)
    set_target_properties(caesium_image_compressor PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist")
endif ()

set_target_properties(caesium_image_compressor PROPERTIES OUTPUT_NAME "Caesium Image Compressor")
target_link_libraries(caesium_image_compressor PRIVATE Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Concurrent Qt5::Svg Qt5::Network zip exiv2lib caesium)

add_subdirectory(tests)
add_test(NAME CsTests COMMAND CsTests)
