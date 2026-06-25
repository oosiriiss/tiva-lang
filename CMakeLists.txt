cmake_minimum_required(VERSION 3.20)

option(TIVA_EXPORT_COMPILE_COMMANDS "Whether to create compile_commands.json" ON)
if(${TIVA_EXPORT_COMPILE_COMMANDS}) 
   set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

project(tiva-lang)

include(FetchContent)
FetchContent_Declare(
   logzy
   GIT_REPOSITORY https://github.com/oosiriiss/logzy
   GIT_TAG 23cd60bcdd4b4e494d4fb46f79ab1860522a1025 # 0.2
)
FetchContent_Declare(
   cppli
   GIT_REPOSITORY https://github.com/oosiriiss/cppli
   GIT_TAG c54463d5987ca076543db42adb5902d5d6901a2e # 0.2.3
)
FetchContent_MakeAvailable(logzy cppli)

# Compiler library
add_library(
   tiva-compiler-lib

   ./src/lexer/lexer.cpp
   ./src/lexer/lexer.hpp
   ./src/lexer/token.hpp
   ./src/utility/utility.hpp

   ./src/parser/parser.cpp
   ./src/parser/parser.hpp
   ./src/parser/ast_nodes.hpp

   ./src/semantic/semantic_visitor.hpp
   ./src/semantic/semantic_visitor.cpp
   ./src/semantic/types.hpp
   ./src/semantic/types.cpp

   ./src/codegen/codegen_visitor.cpp
   ./src/codegen/codegen_visitor.hpp
   ./src/codegen/module.hpp
   ./src/codegen/module.cpp

   ./src/core/translation_unit.hpp
   ./src/core/translation_unit.cpp

   ./src/driver/compiler.cpp
   ./src/driver/compiler.hpp

   ./src/debug.hpp
)


# Adding LLVM
find_package(ZLIB REQUIRED)
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

list(APPEND CMAKE_MODULE_PATH ${LLVM_DIR})
include(AddLLVM)

message(STATUS "LLVM Include dirs: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM definitions: ${LLVM_DEFINITIONS}")


target_include_directories(tiva-compiler-lib PUBLIC src)
if(LLVM_INCLUDE_DIRS) 
   message(STATUS "Adding LLVM include directories to tiva-compiler-lib")
   foreach(LLVM_DIR IN LISTS LLVM_INCLUDE_DIRS)
      # So that my lsp can actually see llvm headers on msys2 windows
      # Everything other that I know of failed
      target_compile_options(tiva-compiler-lib PUBLIC "-idirafter" "${LLVM_DIR}") 
    endforeach()
endif()


target_compile_features(tiva-compiler-lib PUBLIC cxx_std_23)

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
target_compile_options(tiva-compiler-lib PUBLIC ${LLVM_DEFINITIONS_LIST})

if(LLVM_LINK_LLVM_DYLIB)
    message(STATUS "Linking with mono LLVM library")
    set(LLVM_LIBRARIES LLVM)
else()
   message(STATUS "Linking with LLMV components")
   llvm_map_components_to_libnames(LLVM_LIBRARIES core native passes analysis support)
endif()


target_link_libraries(tiva-compiler-lib PUBLIC ${LLVM_LIBRARIES} logzy::logzy cppli::cppli)

message(STATUS "LLVM libs: ${LLVM_LIBRARIES}")


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
