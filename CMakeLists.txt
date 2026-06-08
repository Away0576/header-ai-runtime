cmake_minimum_required(VERSION 3.16)

project(header_ai_runtime VERSION 0.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(DEFAULT_ONNXRUNTIME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/onnxruntime-linux-x64-1.18.1")
set(ONNXRUNTIME_ROOT "${DEFAULT_ONNXRUNTIME_ROOT}" CACHE PATH "ONNX Runtime SDK root directory")
option(HEADER_AI_ENABLE_ONNX "Build ONNX Runtime inference support" OFF)

if(EXISTS "${ONNXRUNTIME_ROOT}/include/onnxruntime_cxx_api.h" AND EXISTS "${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so")
    set(HEADER_AI_ENABLE_ONNX ON CACHE BOOL "Build ONNX Runtime inference support" FORCE)
endif()

add_library(header_ai_runtime_core
    src/MetaConfig.cpp
    src/Normalizer.cpp
    src/ReconstructionError.cpp
    src/SlidingWindow.cpp
)

if(HEADER_AI_ENABLE_ONNX)
    target_sources(header_ai_runtime_core PRIVATE src/OnnxAutoEncoder.cpp)
    target_compile_definitions(header_ai_runtime_core PUBLIC HEADER_AI_ENABLE_ONNX=1)
    target_include_directories(header_ai_runtime_core PUBLIC "${ONNXRUNTIME_ROOT}/include")
    target_link_libraries(header_ai_runtime_core PUBLIC "${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so")
endif()

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

if(HEADER_AI_ENABLE_ONNX)
    set_target_properties(header_ai_detector PROPERTIES
        BUILD_RPATH "${ONNXRUNTIME_ROOT}/lib"
    )
endif()

add_executable(runtime_core_tests
    tests/runtime_core_tests.cpp
)

target_link_libraries(runtime_core_tests
    PRIVATE
        header_ai_runtime_core
)

enable_testing()
add_test(NAME runtime_core_tests COMMAND runtime_core_tests)
