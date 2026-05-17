project(tiva-tests)


include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)
function(Register_Test NAME) 
   add_executable("${NAME}" "${NAME}.cpp")
   target_link_libraries("${NAME}" PRIVATE tiva-compiler-lib GTest::gtest_main)
   add_test(NAME ${NAME} COMMAND ${NAME})
   gtest_discover_tests(${NAME})
endfunction()


Register_Test(lexer_test)
Register_Test(utility_test)
