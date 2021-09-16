cmake_minimum_required(VERSION 3.16)
set(TARGET caesium_image_compressor)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

if (APPLE)
    project(${TARGET} LANGUAGES CXX OBJCXX)
else()
    project(${TARGET} LANGUAGES CXX)
endif()


add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x050F00)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS views)

if (APPLE)
    set(MACOSX_DEPLOYMENT_TARGET 10.14)
    set(MACOSX_BUNDLE_ICON_FILE icon.icns)
    set(CAESIUM_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/icon.icns)
    set_source_files_properties(${CAESIUM_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set(CMAKE_FIND_FRAMEWORK ONLY)
elseif (WIN32)
    set(CAESIUM_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons.rc)
endif ()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wno-deprecated-register -Wno-deprecated")

set(EXIV2_BUILD_SAMPLES OFF)
set(EXIV2_BUILD_EXIV2_COMMAND OFF)
set(EXIV2_ENABLE_XMP OFF)
set(EXIV2_ENABLE_PNG OFF)
if (WIN32)
    set(ZLIB_INCLUDE_DIR "")
endif ()

if (NOT (DEFINED ENV{CMAKE_PREFIX_PATH} OR DEFINED CMAKE_PREFIX_PATH))
    message(FATAL_ERROR "CMAKE_PREFIX_PATH is not defined. Cannot find Qt6 dir.")
endif ()

if (APPLE)
    link_directories(/usr/local/lib)
elseif (WIN32)
    link_directories("C:\\libcaesium\\lib")
    link_directories("C:\\winsparkle\\x64\\Release")
endif ()

find_package(Qt6 COMPONENTS Widgets Core Gui Concurrent Svg Network LinguistTools REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/exiv2)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/include)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/exiv2/include)

if (APPLE)
    include_directories(/Library/Frameworks/Sparkle.framework/Headers)
endif()

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
        src/widgets/QMainCheckBox.cpp
        src/delegates/HtmlDelegate.cpp
        src/exceptions/ImageNotSupportedException.cpp
        src/dialogs/PreferencesDialog.cpp
        )

set(HEADERS
        src/MainWindow.h
        src/models/CImage.h
        src/models/CImageTreeItem.h
        src/models/CImageTreeModel.h
        src/utils/Utils.h
        src/dialogs/AboutDialog.h
        src/widgets/QDropTreeView.h
        src/widgets/QZoomGraphicsView.h
        src/widgets/QMainCheckBox.h
        src/delegates/HtmlDelegate.h
        src/exceptions/ImageNotSupportedException.h
        src/dialogs/PreferencesDialog.h
        )

if (APPLE)
    set(OSX_SOURCES
            src/updater/osx/AutoUpdater.cpp
            src/updater/osx/CocoaInitializer.mm
            src/updater/osx/SparkleAutoUpdater.mm
            )
    set(OSX_HEADERS
            src/updater/osx/AutoUpdater.h
            src/updater/osx/SparkleAutoUpdater.h
            )

else()
    set(WIN_HEADERS
            src/updater/win/winsparkle.h
#            src/updater/win/winsparkle-version.h
            )
endif ()

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
    add_executable(${TARGET} WIN32 ${SOURCES} ${HEADERS} ${WIN_HEADERS} ${UI} ${RESOURCES} ${CAESIUM_ICON} ${QM_FILES})
else ()
    add_executable(${TARGET} MACOSX_BUNDLE ${SOURCES} ${OSX_SOURCES} ${HEADERS} ${OSX_HEADERS} ${UI} ${RESOURCES} ${CAESIUM_ICON} ${QM_FILES})
endif ()

if (APPLE)
    set(OSX_FRAMEWORKS "-framework AppKit -framework Carbon -framework Foundation -framework ApplicationServices -framework Sparkle")
    set_target_properties(${TARGET} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist" LINK_FLAGS "-F/Library/Frameworks -Wl -rpath @loader_path/../Frameworks")
endif ()

set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME "Caesium Image Compressor")
if (APPLE)
    target_link_libraries(${TARGET} PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Concurrent Qt6::Svg Qt6::Network ${OSX_FRAMEWORKS} exiv2lib caesium)
else ()
    target_link_libraries(${TARGET} PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Concurrent Qt6::Svg Qt6::Network exiv2lib winsparkle caesium)
endif ()

add_subdirectory(tests)
add_test(NAME CsTests COMMAND CsTests)
