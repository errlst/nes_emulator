cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 23)

project(nes_emulator)

add_subdirectory(3rd/imgui)
add_subdirectory(nes)

include_directories(3rd/imgui 3rd/glfw)

add_executable(exec main.cpp)

target_link_directories(exec PRIVATE 3rd/glfw)

target_link_libraries(exec PRIVATE glfw imgui GL nes)

