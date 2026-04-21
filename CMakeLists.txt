cmake_minimum_required(VERSION 3.13)
project(ast_compare_project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Clang REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})

add_definitions(${LLVM_DEFINITIONS})
add_definitions(${CLANG_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs support core irreader)

add_library(ast_compare_lib ast_compare.cpp)
target_link_libraries(ast_compare_lib
  clangTooling clangFrontend clangDriver clangSerialization
  clangParse clangSema clangAnalysis clangAST clangBasic
  clangEdit clangLex clangRewrite ${llvm_libs}
)

add_executable(ast_compare cli.cpp)
target_link_libraries(ast_compare ast_compare_lib)
