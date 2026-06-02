cmake_minimum_required(VERSION 3.20)
project(metastr-cpp VERSION 0.3.6 LANGUAGES CXX)

option(METASTR_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(METASTR_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(METASTR_BUILD_BENCHMARKS "Build size benchmark executables" OFF)
option(METASTR_GENERATE_BUILD_KEY "Generate a per-build MetaStr key header" ON)

set(METASTR_GENERATED_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(METASTR_GENERATED_CONFIG ${METASTR_GENERATED_INCLUDE_DIR}/metastr/metastr_build_config.hpp)

if(METASTR_GENERATE_BUILD_KEY)
    find_program(METASTR_POWERSHELL NAMES pwsh powershell powershell.exe)
    if(NOT METASTR_POWERSHELL AND WIN32 AND DEFINED ENV{SystemRoot})
        set(METASTR_WINDOWS_POWERSHELL "$ENV{SystemRoot}/System32/WindowsPowerShell/v1.0/powershell.exe")
        if(EXISTS ${METASTR_WINDOWS_POWERSHELL})
            set(METASTR_POWERSHELL ${METASTR_WINDOWS_POWERSHELL})
        endif()
    endif()
    if(METASTR_POWERSHELL)
        execute_process(
            COMMAND ${METASTR_POWERSHELL} -NoProfile -ExecutionPolicy Bypass -File
                ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_build_key.ps1
                -Output ${METASTR_GENERATED_CONFIG}
            RESULT_VARIABLE METASTR_KEY_RESULT
        )
    endif()
endif()

if(NOT EXISTS ${METASTR_GENERATED_CONFIG})
    file(MAKE_DIRECTORY ${METASTR_GENERATED_INCLUDE_DIR}/metastr)
    file(WRITE ${METASTR_GENERATED_CONFIG}
"#pragma once\n\n#define METASTR_HAS_BUILD_KEY 0\n#define METASTR_BUILD_KEY0 0x4b6d7a9c1e3052f1ull\n#define METASTR_BUILD_KEY1 0x9f27c4b8d65a103eull\n#define METASTR_BUILD_KEY2 0x2d8c71e5a493bf60ull\n#define METASTR_BUILD_KEY3 0xf13b6a90c72e5d48ull\n")
    message(WARNING "metastr-cpp is using the deterministic fallback build key; generated builds are easier to correlate")
endif()

add_library(metastr-cpp INTERFACE)
add_library(metastr::metastr ALIAS metastr-cpp)
target_compile_features(metastr-cpp INTERFACE cxx_std_20)
target_include_directories(metastr-cpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${METASTR_GENERATED_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

if(WIN32)
    target_compile_definitions(metastr-cpp INTERFACE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

if(METASTR_BUILD_EXAMPLES)
    add_executable(metastr-basic examples/basic.cpp)
    target_link_libraries(metastr-basic PRIVATE metastr::metastr)

    add_executable(metastr-scoped examples/scoped_decode.cpp)
    target_link_libraries(metastr-scoped PRIVATE metastr::metastr)

    add_executable(metastr-manual-blob examples/manual_blob.cpp)
    target_link_libraries(metastr-manual-blob PRIVATE metastr::metastr)

    add_executable(metastr-app-strings examples/app_strings.cpp)
    target_link_libraries(metastr-app-strings PRIVATE metastr::metastr)
endif()

if(METASTR_BUILD_TESTS)
    enable_testing()
    add_executable(metastr-tests tests/test_cases.cpp)
    target_link_libraries(metastr-tests PRIVATE metastr::metastr)
    add_test(NAME metastr-tests COMMAND metastr-tests)
endif()

if(METASTR_BUILD_BENCHMARKS)
    add_executable(metastr-bench-baseline benchmarks/baseline.cpp)
    target_include_directories(metastr-bench-baseline PRIVATE benchmarks)
    target_compile_features(metastr-bench-baseline PRIVATE cxx_std_20)

    add_executable(metastr-bench-stream benchmarks/metastr_stream.cpp)
    target_include_directories(metastr-bench-stream PRIVATE benchmarks)
    target_compile_features(metastr-bench-stream PRIVATE cxx_std_20)
    target_link_libraries(metastr-bench-stream PRIVATE metastr::metastr)

    add_executable(metastr-bench-auto benchmarks/metastr_auto.cpp)
    target_include_directories(metastr-bench-auto PRIVATE benchmarks)
    target_compile_features(metastr-bench-auto PRIVATE cxx_std_20)
    target_link_libraries(metastr-bench-auto PRIVATE metastr::metastr)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS metastr-cpp EXPORT metastr-cpp-targets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY ${METASTR_GENERATED_INCLUDE_DIR}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT metastr-cpp-targets
    NAMESPACE metastr::
    FILE metastr-cpp-targets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/metastr-cpp
)

configure_package_config_file(
    cmake/metastr-cpp-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/metastr-cpp-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/metastr-cpp
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/metastr-cpp-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/metastr-cpp-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/metastr-cpp-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/metastr-cpp
)
