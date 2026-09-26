cmake_minimum_required(VERSION 3.20)
project(DLOpen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Compile mathlib.cpp into a shared library
add_library(mathlib SHARED mathlib.cpp)
set_target_properties(mathlib PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(dlopen_runner main.cpp dlopen.cpp)

target_include_directories(dlopen_runner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(dlopen_runner PRIVATE ${CMAKE_DL_LIBS})

target_compile_definitions(dlopen_runner PRIVATE
    MATHLIB_SHARED_NAME="$<TARGET_FILE_NAME:mathlib>"
)

# Compile other dynamic library for test cases
add_library(testcases SHARED testcases.cpp)
set_target_properties(testcases PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(dynamic_runner runner.cpp)
target_link_libraries(dynamic_runner PRIVATE ${CMAKE_DL_LIBS})

add_dependencies(dlopen_runner mathlib)