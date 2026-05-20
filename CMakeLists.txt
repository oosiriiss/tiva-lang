cmake_minimum_required(VERSION 3.20)

option(TIVA_EXPORT_COMPILE_COMMANDS "Whether to create compile_commands.json" ON)
if(${TIVA_EXPORT_COMPILE_COMMANDS}) 
   set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

project(tiva-lang)

# Compiler library
add_library(
   tiva-compiler-lib

   ./src/lexer.cpp
   ./src/lexer.hpp
   ./src/utility.hpp

   ./src/parser/parser.cpp
   ./src/parser/parser.hpp
   ./src/parser/ast_nodes.hpp

   ./src/debug.hpp
)
target_include_directories(tiva-compiler-lib PUBLIC src)
target_compile_features(tiva-compiler-lib PUBLIC cxx_std_23)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
   target_link_libraries(tiva-compiler-lib PUBLIC stdc++exp)
endif()


# Compiler executable
add_executable(
   tiva-compiler
   ./src/main.cpp
)
target_include_directories(tiva-compiler PRIVATE src)
target_link_libraries(tiva-compiler PRIVATE tiva-compiler-lib)


option(TIVA_COMPILE_TESTS "Whether to compile Tiva compiler tests" ON)
if(${TIVA_COMPILE_TESTS}) 

   include(CTest)
   enable_testing()

   add_subdirectory(./test/)

endif()


include(./cmake/generate-dot-clangd.cmake)
