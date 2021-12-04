cmake_minimum_required(VERSION 3.16)

project (ray LANGUAGES C CXX)
if (NOT "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    set(Prg-Intel "PrgEnv-intel")
    set(Prg-Clang "PrgEnv-cray")
    message(WARNING
            "Must use GNU Compiler for submission. Make sure you ran:\n"
            "module swap ${Prg-${CMAKE_C_COMPILER_ID}} PrgEnv-gnu")
endif ()

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS "-Wall -Wno-unused-function")

add_executable (ray ray.cpp)
target_compile_features(ray PRIVATE cxx_std_11)
target_compile_options(ray PRIVATE -fopenmp)
target_link_options(ray PRIVATE -fopenmp)

add_executable (imagewriter imagewriter.cpp)
target_link_libraries(imagewriter m)

add_executable (scenewriter scenewriter.cpp)
target_link_libraries(scenewriter m)
