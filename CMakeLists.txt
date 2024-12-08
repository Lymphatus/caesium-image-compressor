cmake_minimum_required(VERSION 3.16)
include(ExternalProject)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(TARGET caesium_image_compressor)
file(STRINGS "libcaesium.conf" LIBCAESIUM_VERSION)
if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    if (DEFINED PORTABLE AND PORTABLE)
        message("-- Configuring for portable")
        add_compile_definitions(IS_PORTABLE)
    endif ()
elseif (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wno-deprecated-register -Wno-deprecated")
else ()
    include(GNUInstallDirs)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wno-deprecated-register -Wno-deprecated")
endif ()
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11")
    project(${TARGET} LANGUAGES CXX OBJCXX)
else ()
    project(${TARGET} LANGUAGES CXX)
endif ()

find_library(LIBCAESIUM caesium)
if (NOT LIBCAESIUM)
    find_program(CARGO "cargo" REQUIRED)
endif ()

add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x050F00)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS views)

if (APPLE)
    set(MACOSX_BUNDLE_ICON_FILE icon.icns)
    set(CAESIUM_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/icon.icns)
    set_source_files_properties(${CAESIUM_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
elseif (WIN32)
    set(CAESIUM_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons.rc)
endif ()

if (NOT (DEFINED ENV{CMAKE_PREFIX_PATH} OR DEFINED CMAKE_PREFIX_PATH))
    message(FATAL_ERROR "CMAKE_PREFIX_PATH is not defined. Cannot find Qt dir.")
endif ()

if (WIN32)
    set(WINSPARKLE_LIB_DIR ${CMAKE_BINARY_DIR}/libwinsparkle-prefix/src/libwinsparkle/x64/Release)
    link_directories(${WINSPARKLE_LIB_DIR})
endif ()

if (APPLE)
    if (NOT DEFINED LIBSSH_INCLUDE_DIR)
        message(FATAL_ERROR "LIBSSH_INCLUDE_DIR is not defined. Cannot find libssh include dir.")
    else ()
        include_directories(${LIBSSH_INCLUDE_DIR})
    endif ()

    if (NOT DEFINED SPARKLE_INCLUDE_DIR)
        message(FATAL_ERROR "SPARKLE_INCLUDE_DIR is not defined. Cannot find Sparkle include dir.")
    else ()
        include_directories(${SPARKLE_INCLUDE_DIR})
    endif ()
endif ()

set(LIBCAESIUM_SOURCE_DIR ${CMAKE_BINARY_DIR}/libcaesium-prefix/src/libcaesium)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif ()

string(TOLOWER ${CMAKE_BUILD_TYPE} LIBCAESIUM_BUILD_TYPE)

if (NOT WIN32)
    link_directories(${LIBCAESIUM_SOURCE_DIR}/target/${LIBCAESIUM_BUILD_TYPE})
endif ()
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Core Gui Concurrent Svg Network Sql LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Core Gui Concurrent Svg Network Sql LinguistTools REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/include)

set(SOURCES
        src/main.cpp
        src/MainWindow.cpp
        src/models/CImage.cpp
        src/models/CImageTreeItem.cpp
        src/models/CImageTreeModel.cpp
        src/models/CImageSortFilterProxyModel.cpp
        src/utils/Utils.cpp
        src/utils/LanguageManager.cpp
        src/utils/Logger.cpp
        src/utils/PostCompressionActions.cpp
        src/dialogs/AboutDialog.cpp
        src/widgets/QDropTreeView.cpp
        src/widgets/QDropListWidget.cpp
        src/widgets/QZoomGraphicsView.cpp
        src/widgets/QCaesiumMessageBox.cpp
        src/widgets/QMainCheckBox.cpp
        src/widgets/QLoadingLabel.cpp
        src/widgets/QCollapseToolButton.cpp
        src/delegates/HtmlDelegate.cpp
        src/exceptions/ImageNotSupportedException.cpp
        src/exceptions/ImageTooBigException.cpp
        src/dialogs/PreferencesDialog.cpp
        src/network/NetworkOperations.cpp
        src/database/Database.cpp
        src/dialogs/UsageStatsDialog.cpp
        src/dialogs/AdvancedImportDialog.cpp
        src/services/Importer.cpp
        src/filters/QSliderScrollFilter.cpp
)

if (APPLE)
    set(OSX_SOURCES
            src/updater/osx/AutoUpdater.cpp
            src/updater/osx/CocoaInitializer.mm
            src/updater/osx/SparkleAutoUpdater.mm
    )
endif ()

set(UI
        views/MainWindow.ui
        views/AboutDialog.ui
        views/PreferencesDialog.ui
        views/UsageStatsDialog.ui
        views/AdvancedImportDialog.ui
)

SET(TS_FILES
        resources/i18n/caesium_en_US.ts
        resources/i18n/caesium_it_IT.ts
        resources/i18n/caesium_fr_FR.ts
        resources/i18n/caesium_hi_IN.ts
        resources/i18n/caesium_zh_CN.ts
        resources/i18n/caesium_ja_JP.ts
        resources/i18n/caesium_es_ES.ts
        resources/i18n/caesium_ar_EG.ts
        resources/i18n/caesium_de_DE.ts
        resources/i18n/caesium_ru_RU.ts
        resources/i18n/caesium_tr_TR.ts
        resources/i18n/caesium_pt_BR.ts
        resources/i18n/caesium_el_GR.ts
        resources/i18n/caesium_id_ID.ts
        resources/i18n/caesium_zh_TW.ts
        resources/i18n/caesium_sk_SK.ts
        resources/i18n/caesium_pl_PL.ts
        resources/i18n/caesium_fi_FI.ts
)

set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION ${CMAKE_BINARY_DIR}/i18n)
if (QT_VERSION_MAJOR EQUAL "6")
    qt6_create_translation(QM_FILES
            ${CMAKE_SOURCE_DIR}
            ${TS_FILES})
else ()
    qt5_create_translation(QM_FILES
            ${CMAKE_SOURCE_DIR}
            ${TS_FILES})
endif ()
configure_file(resources/languages.qrc ${CMAKE_BINARY_DIR} COPYONLY)

set(RESOURCES
        ${CMAKE_BINARY_DIR}/languages.qrc
        resources/resources.qrc)

if (WIN32)
    add_executable(${TARGET} WIN32 ${SOURCES} ${UI} ${RESOURCES} ${CAESIUM_ICON} ${QM_FILES})
elseif (APPLE)
    add_executable(${TARGET} MACOSX_BUNDLE ${SOURCES} ${OSX_SOURCES} ${UI} ${RESOURCES} ${CAESIUM_ICON} ${QM_FILES})
else ()
    add_executable(${TARGET} ${SOURCES} ${OSX_SOURCES} ${UI} ${RESOURCES} ${CAESIUM_ICON} ${QM_FILES})
    install(TARGETS ${TARGET} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    if (NOT LIBCAESIUM)
        install(
                FILES ${LIBCAESIUM_SOURCE_DIR}/target/${LIBCAESIUM_BUILD_TYPE}/libcaesium.so
                DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        )
        install(
                CODE "execute_process(
                COMMAND ${CMAKE_COMMAND} -E create_symlink
                ${CMAKE_INSTALL_PREFIX}/lib/libcaesium.so
                /usr/lib/libcaesium.so
            )"
        )
    endif ()
endif ()

# --- libcaesium ---
if (CMAKE_BUILD_TYPE MATCHES Debug)
    set(CARGO_BUILD_FLAGS "")
else ()
    set(CARGO_BUILD_FLAGS "--release")
endif ()

set(CARGO_TARGET "")
if (WIN32)
    set(CARGO_TARGET "--target=x86_64-pc-windows-gnu")
    set(LIBCAESIUM_EXTERNAL_PATH ${LIBCAESIUM_SOURCE_DIR}/target/x86_64-pc-windows-gnu/${LIBCAESIUM_BUILD_TYPE}/caesium.dll)
else ()
    set(LIBCAESIUM_EXTERNAL_PATH ${LIBCAESIUM_SOURCE_DIR}/target/${LIBCAESIUM_BUILD_TYPE}/libcaesium.so)
endif ()

if (LIBCAESIUM)
    target_link_libraries(${TARGET} PRIVATE ${LIBCAESIUM})
else ()
    ExternalProject_Add(
            libcaesium
            GIT_REPOSITORY https://github.com/Lymphatus/libcaesium
            GIT_TAG ${LIBCAESIUM_VERSION}
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ${CARGO} build ${CARGO_BUILD_FLAGS} ${CARGO_TARGET}
            INSTALL_COMMAND ""
            BINARY_DIR ${LIBCAESIUM_SOURCE_DIR}
            LOG_OUTPUT_ON_FAILURE ON
            LOG_BUILD ON
            BUILD_BYPRODUCTS ${LIBCAESIUM_EXTERNAL_PATH}
    )
    add_dependencies(${TARGET} libcaesium)
endif ()

# --- WinSparkle ---
if (WIN32)
    ExternalProject_Add(
            libwinsparkle
            URL https://github.com/vslavik/winsparkle/releases/download/v0.7.0/WinSparkle-0.7.0.zip
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND cmake -E echo "Skipping install step."
            LOG_DOWNLOAD OFF
            LOG_OUTPUT_ON_FAILURE ON
            DOWNLOAD_EXTRACT_TIMESTAMP ON
    )
    add_dependencies(caesium_image_compressor libwinsparkle)
endif ()
# --- WinSparkle ---

if (APPLE)
    set(OSX_FRAMEWORKS "-framework AppKit -framework Carbon -framework Foundation -framework ApplicationServices -framework Sparkle")
    set_target_properties(${TARGET} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist" LINK_FLAGS "-F/Library/Frameworks -Wl -rpath @loader_path/../Frameworks")
endif ()

set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME "Caesium Image Compressor")
if (APPLE)
    target_link_libraries(${TARGET} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Concurrent Qt${QT_VERSION_MAJOR}::Svg Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Sql ${OSX_FRAMEWORKS} caesium)
elseif (WIN32)
    target_link_libraries(${TARGET} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Concurrent Qt${QT_VERSION_MAJOR}::Svg Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Sql winsparkle ${LIBCAESIUM_SOURCE_DIR}/target/x86_64-pc-windows-gnu/${LIBCAESIUM_BUILD_TYPE}/caesium.dll)
else ()
    set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME "caesium-image-compressor")
    target_link_libraries(${TARGET} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Concurrent Qt${QT_VERSION_MAJOR}::Svg Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Sql)
    if (NOT LIBCAESIUM)
        target_link_libraries(${TARGET} PRIVATE caesium)
    endif ()
endif ()

add_subdirectory(tests)
add_test(NAME CsTests COMMAND CsTests)
