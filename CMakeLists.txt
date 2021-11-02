cmake_minimum_required(VERSION 3.20)
project(tobsterlang)

find_package(Boost)
find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs_core)

set(CMAKE_CXX_STANDARD 17)

add_executable(tobsterlang main.cpp)

target_link_libraries(tobsterlang "LLVM-${LLVM_PACKAGE_VERSION}")