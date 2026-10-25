cmake_minimum_required(VERSION 3.20)
project(ProjectLauncher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Add the separate projects
add_subdirectory(dlopen)
add_subdirectory(74-wsserver)
#add_subdirectory(ioctl)

add_executable(launcher main.cpp)

target_compile_definitions(launcher PRIVATE
    DLOPEN_NAME="$<TARGET_FILE_NAME:dlopen>"
    WSSERVER_NAME="$<TARGET_FILE_NAME:wsserver>"
)