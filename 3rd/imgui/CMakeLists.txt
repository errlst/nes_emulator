cmake_minimum_required(VERSION 3.28)

project(imgui)

file(GLOB cpp_files "*.cpp")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../glfw)

add_library(imgui STATIC ${cpp_files})