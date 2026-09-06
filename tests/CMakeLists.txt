cmake_minimum_required(VERSION 3.5)
project(CsTests LANGUAGES CXX)
find_package(Qt6 COMPONENTS Test REQUIRED)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()
# Importer.cpp now builds CImage objects, so the test target needs the model sources and libcaesium.
add_executable(CsTests
    main.cpp ImporterTests.cpp ImporterTests.h
    ../src/services/Importer.cpp
    ../src/models/CImage.cpp
    ../src/utils/Utils.cpp
    ../src/widgets/QCaesiumMessageBox.cpp
    ../src/exceptions/ImageNotSupportedException.cpp
    ../src/exceptions/ImageTooBigException.cpp)
target_include_directories(CsTests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/include)
target_link_libraries(CsTests PRIVATE Qt6::Test Qt6::Widgets Qt6::Core Qt6::Gui
    Qt6::Concurrent Qt6::Svg Qt6::Network)
add_dependencies(CsTests libcaesium)
if(WIN32)
    target_link_libraries(CsTests PRIVATE ${LIBCAESIUM_EXTERNAL_PATH})
    add_custom_command(TARGET CsTests POST_BUILD COMMAND ${CMAKE_COMMAND} -E
        copy_if_different ${LIBCAESIUM_EXTERNAL_PATH} $<TARGET_FILE_DIR:CsTests>)
else()
    target_link_libraries(CsTests PRIVATE caesium)
endif()
add_test(NAME CsTests COMMAND CsTests)
set_tests_properties(CsTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
