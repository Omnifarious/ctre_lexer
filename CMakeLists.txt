cmake_minimum_required (VERSION 3.30)

project(ctre_test)

add_subdirectory(Catch2)
set(CMAKE_MODULE_PATH
        ${CMAKE_MODULE_PATH}
        ${CMAKE_CURRENT_LIST_DIR}/Catch2/contrib
)

# Enable AddressSanitizer for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling AddressSanitizer for Debug build")
    add_compile_options(-fsanitize=address,undefined  -fsanitize-address-use-after-scope -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined -fsanitize-address-use-after-scope)
endif()

add_subdirectory(ctre)

find_package(fmt REQUIRED)

add_executable(lexer_test lexer_test.cpp
        parser.cpp)
target_compile_features(lexer_test PUBLIC cxx_std_23)
target_link_libraries(lexer_test PRIVATE ctre::ctre)

add_executable(all_tests tests/tokenizer_test.cpp tests/ring_buf_test.cpp
        tests/parser_test.cpp tests/while_test.cpp parser.cpp)
target_compile_features(all_tests PUBLIC cxx_std_23)
target_link_libraries(all_tests PRIVATE Catch2::Catch2WithMain ctre::ctre)

include(CTest)
include(Catch)
catch_discover_tests(all_tests)
