cmake_minimum_required(VERSION 3.5)
project(CsTests LANGUAGES CXX)
find_package(Qt6 COMPONENTS Test REQUIRED)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(CAESIUM_TESTS_NEW_API "Build tests for the large-batch public API" ON)
enable_testing()
add_executable(CsTests
    main.cpp ImporterTests.cpp CImageTreeModelTests.cpp SortProxyTests.cpp
    CImageCompressTests.cpp LoggerTests.cpp
    ../src/services/Importer.cpp
    ../src/models/CImage.cpp ../src/models/CImageTreeItem.cpp
    ../src/models/CImageTreeModel.cpp ../src/models/CImageSortFilterProxyModel.cpp
    ../src/utils/Utils.cpp ../src/utils/Logger.cpp
    ../src/widgets/QCaesiumMessageBox.cpp
    ../src/exceptions/ImageNotSupportedException.cpp
    ../src/exceptions/ImageTooBigException.cpp
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc)
if(CAESIUM_TESTS_NEW_API)
    target_sources(CsTests PRIVATE UtilsTests.cpp ImporterBuildImagesTests.cpp
        NewApiModelTests.cpp CompressionFlowTests.cpp StressTests.cpp ProcessMemory.cpp)
    target_compile_definitions(CsTests PRIVATE CAESIUM_TESTS_NEW_API)
endif()
target_include_directories(CsTests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/include)
target_link_libraries(CsTests PRIVATE Qt6::Test Qt6::Widgets Qt6::Core Qt6::Gui
    Qt6::Concurrent Qt6::Svg Qt6::Network)
add_dependencies(CsTests libcaesium)
if(WIN32)
    target_link_libraries(CsTests PRIVATE ${LIBCAESIUM_EXTERNAL_PATH} psapi)
    add_custom_command(TARGET CsTests POST_BUILD COMMAND ${CMAKE_COMMAND} -E
        copy_if_different ${LIBCAESIUM_EXTERNAL_PATH} $<TARGET_FILE_DIR:CsTests>)
else()
    target_link_libraries(CsTests PRIVATE caesium)
endif()
add_test(NAME CsTests COMMAND CsTests -o -,txt)
set_tests_properties(CsTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 3600)
if(WIN32)
    get_target_property(QT_CORE_DLL Qt6::Core IMPORTED_LOCATION_RELEASE)
    get_filename_component(QT_BIN_DIR "${QT_CORE_DLL}" DIRECTORY)
    set_tests_properties(CsTests PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:${QT_BIN_DIR};PATH=path_list_prepend:$<TARGET_FILE_DIR:CsTests>")
endif()
