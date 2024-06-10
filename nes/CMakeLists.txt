project(nes)

file(GLOB cpp_files "*.cpp")

add_library(nes STATIC ${cpp_files})