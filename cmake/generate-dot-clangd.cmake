## Auto create .clangd file if it doesnt' exist
get_filename_component(COMPILER_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY) # bin directory
get_filename_component(COMPILER_ROOT ${COMPILER_BIN_DIR} DIRECTORY) # main compiler directory
set(COMPILER_INCLUDE_DIR "${COMPILER_ROOT}/include") # compiler include directory

set(CLANGD_GENERATED_FILE "${CMAKE_BINARY_DIR}/.clangd.generated")
set(CLANGD_DESTINATION "${CMAKE_SOURCE_DIR}/.clangd")

file(WRITE "${CLANGD_GENERATED_FILE}" 
"CompileFlags:\n"
"  CompilationDatabase: ./build\n"
"  Add:\n"
"    - \"-isystem\"\n"
"    - \"${COMPILER_INCLUDE_DIR}\"\n"
)

add_custom_target(generate-clangd
    COMMAND ${CMAKE_COMMAND} -E copy "${CLANGD_GENERATED_FILE}" "${CLANGD_DESTINATION}"
    COMMENT "Generate .clangd in the project's root directory"
)

if(NOT EXISTS "${CLANGD_FILE}")
   message(STATUS "Autocreating .clangd")
    execute_process(
	COMMAND ${CMAKE_COMMAND} -E copy "${CLANGD_GENERATED_FILE}" "${CLANGD_DESTINATION}"
    )
endif()
