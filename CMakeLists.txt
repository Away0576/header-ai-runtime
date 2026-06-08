cmake_minimum_required(VERSION 3.16)

project(header_ai_runtime VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(header_ai_runtime_core
    src/MetaConfig.cpp
    src/Normalizer.cpp
    src/SlidingWindow.cpp
)

target_include_directories(header_ai_runtime_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(header_ai_runtime_core
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

add_executable(header_ai_detector
    src/main.cpp
)

target_link_libraries(header_ai_detector
    PRIVATE
        header_ai_runtime_core
)

add_executable(runtime_core_tests
    tests/runtime_core_tests.cpp
)

target_link_libraries(runtime_core_tests
    PRIVATE
        header_ai_runtime_core
)

enable_testing()
add_test(NAME runtime_core_tests COMMAND runtime_core_tests)
