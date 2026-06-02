cmake_minimum_required(VERSION 3.20)
project(metastr-cpp VERSION 0.1.0 LANGUAGES CXX)

option(METASTR_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(METASTR_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})

add_library(metastr-cpp INTERFACE)
add_library(metastr::metastr ALIAS metastr-cpp)
target_compile_features(metastr-cpp INTERFACE cxx_std_20)
target_include_directories(metastr-cpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(WIN32)
    target_compile_definitions(metastr-cpp INTERFACE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

if(METASTR_BUILD_EXAMPLES)
    add_executable(metastr-basic examples/basic.cpp)
    target_link_libraries(metastr-basic PRIVATE metastr::metastr)
endif()

if(METASTR_BUILD_TESTS)
    enable_testing()
    add_executable(metastr-tests tests/test_cases.cpp)
    target_link_libraries(metastr-tests PRIVATE metastr::metastr)
    add_test(NAME metastr-tests COMMAND metastr-tests)
endif()
