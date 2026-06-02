cmake_minimum_required(VERSION 3.20)
project(metastr-consumer LANGUAGES CXX)

find_package(metastr-cpp CONFIG REQUIRED)

add_executable(metastr-consumer consumer.cpp)
target_link_libraries(metastr-consumer PRIVATE metastr::metastr-cpp)
