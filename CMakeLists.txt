cmake_minimum_required (VERSION 3.17)

project(ctre_test)

add_subdirectory(ctre)

add_executable(ctre_test ctre_test.cpp)
target_compile_features(ctre_test PUBLIC cxx_std_20)
target_compile_options(ctre_test PUBLIC -fcoroutines)
target_link_libraries(ctre_test ctre::ctre)

add_executable(generic_hash generic_hash.cpp)
target_compile_features(generic_hash PUBLIC cxx_std_14)
