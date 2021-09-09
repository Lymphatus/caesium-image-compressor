cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

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
elseif (WIN32)
    set(CAESIUM_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons.rc)
endif ()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wno-deprecated-register")

set(EXIV2_BUILD_SAMPLES OFF)
set(EXIV2_BUILD_EXIV2_COMMAND OFF)
set(EXIV2_ENABLE_XMP OFF)
set(EXIV2_ENABLE_PNG OFF)
if(WIN32)
 set(ZLIB_INCLUDE_DIR "")
endif()

if (NOT (DEFINED ENV{CMAKE_PREFIX_PATH} OR DEFINED CMAKE_PREFIX_PATH))
    message(FATAL_ERROR "CMAKE_PREFIX_PATH is not defined. Cannot find Qt5 dir.")
endif ()

if (APPLE)
    link_directories(/usr/local/lib)
elseif (WIN32)
    link_directories("C:\\libcaesium\\lib")
endif ()

find_package(Qt6 COMPONENTS Widgets Core Gui Concurrent Svg Network LinguistTools REQUIRED)

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
        src/utils/Updater.cpp
        src/dialogs/PreferencesDialog.cpp)

set(HEADERS
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
        src/utils/Updater.h
        src/dialogs/PreferencesDialog.h)

set(UI
        views/MainWindow.ui
        views/AboutDialog.ui
        views/PreferencesDialog.ui)

SET(TS_FILES
        resources/i18n/caesium_en_US.ts
        resources/i18n/caesium_it_IT.ts)

set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION ${CMAKE_BINARY_DIR}/i18n)
qt6_create_translation(QM_FILES
        ${CMAKE_SOURCE_DIR}
        ${TS_FILES})

configure_file(resources/languages.qrc ${CMAKE_BINARY_DIR} COPYONLY)
set(RESOURCES
        ${CMAKE_BINARY_DIR}/languages.qrc
        resources/resources.qrc)

if (WIN32)
    add_executable(caesium_image_compressor WIN32 ${SOURCES} ${HEADERS} ${UI} ${RESOURCES} ${CAESIUM_ICON} ${QM_FILES})
else ()
    add_executable(caesium_image_compressor MACOSX_BUNDLE ${SOURCES} ${HEADERS} ${UI} ${RESOURCES} ${CAESIUM_ICON} ${QM_FILES})
endif ()
if (APPLE)
    set_target_properties(caesium_image_compressor PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist")
endif ()

set_target_properties(caesium_image_compressor PROPERTIES OUTPUT_NAME "Caesium Image Compressor")
target_link_libraries(caesium_image_compressor PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Concurrent Qt6::Svg Qt6::Network zip exiv2lib caesium)

add_subdirectory(tests)
add_test(NAME CsTests COMMAND CsTests)
