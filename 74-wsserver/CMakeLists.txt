cmake_minimum_required(VERSION 3.20)
project(WSServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(wsserver main.cpp websocket.cpp)

target_include_directories(wsserver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
