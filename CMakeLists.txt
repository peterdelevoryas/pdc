cmake_minimum_required(VERSION 3.19)
project(pd)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "LLVM package version: ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM directory: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(pdc main.c)
target_compile_options(pdc PRIVATE -Wall -g -O1)

llvm_map_components_to_libnames(llvm_libs support core x86codegen)
target_link_libraries(pdc ${llvm_libs})
