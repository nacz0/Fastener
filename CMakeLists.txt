cmake_minimum_required(VERSION 3.16)
project(Fastener VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(FST_BUILD_EXAMPLES "Build example applications" ON)
option(FST_BUILD_TESTS "Build tests" OFF)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Collect source files
file(GLOB_RECURSE FST_SOURCES 
    "src/*.cpp"
)

file(GLOB_RECURSE FST_HEADERS
    "include/*.h"
)

# Create library
add_library(fastener STATIC ${FST_SOURCES} ${FST_HEADERS})

target_include_directories(fastener PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(fastener PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)

target_link_libraries(fastener PUBLIC
    OpenGL::GL
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(fastener PUBLIC
        dwmapi
        gdi32
        user32
    )
elseif(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(fastener PUBLIC
        ${X11_LIBRARIES}
        GL
    )
    target_include_directories(fastener PRIVATE ${X11_INCLUDE_DIR})
endif()

# Examples
if(FST_BUILD_EXAMPLES)
    add_executable(fastener_demo examples/demo/main.cpp)
    target_link_libraries(fastener_demo PRIVATE fastener)
    
    add_executable(multi_window_demo examples/multi_window/main.cpp)
    target_link_libraries(multi_window_demo PRIVATE fastener)
endif()

# Install
install(TARGETS fastener
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

# Tests
if(FST_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # Prevent overriding the parent project's compiler/linker settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    enable_testing()
    add_executable(fastener_tests 
        tests/test_types.cpp
        tests/test_date_time.cpp
        tests/test_widget_utils.cpp
        tests/test_layout.cpp
        tests/test_input.cpp
        tests/test_docking.cpp
        tests/test_widget_rendering.cpp
        tests/test_context_di.cpp
        tests/test_i18n.cpp
        tests/test_rich_text_preview.cpp
        tests/test_text_area.cpp
        tests/test_svg.cpp
        tests/test_svg_widget.cpp
    )
    target_link_libraries(fastener_tests PRIVATE fastener GTest::gtest_main GTest::gmock)
    include(GoogleTest)
    gtest_discover_tests(fastener_tests)
endif()
