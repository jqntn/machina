cmake_minimum_required(VERSION 3.15)

project(machina LANGUAGES CXX)

find_package(fmt CONFIG REQUIRED)
find_package(raylib CONFIG REQUIRED)

add_executable(machina)

set_property(TARGET machina PROPERTY CXX_STANDARD 20)
set_property(TARGET machina PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET machina PROPERTY CXX_EXTENSIONS ON)

file(GLOB_RECURSE src "src/*.cpp")
target_sources(machina PRIVATE "${src}")

target_include_directories(machina PRIVATE "include")

target_link_libraries(machina PRIVATE fmt::fmt)
target_link_libraries(machina PRIVATE raylib)