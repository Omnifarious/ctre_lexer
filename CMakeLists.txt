cmake_minimum_required (VERSION 3.13)

project(ctre_test)

add_subdirectory(ctre)

add_executable(ctre_test ctre_test.cpp)
target_compile_features(ctre_test PUBLIC cxx_std_17)
target_link_libraries(ctre_test ctre)
