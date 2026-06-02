cmake_minimum_required(VERSION 3.20)
project(metastr-cpp VERSION 0.3.4 LANGUAGES CXX)

option(METASTR_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(METASTR_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(METASTR_BUILD_BENCHMARKS "Build size benchmark executables" OFF)

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
