cmake_minimum_required(VERSION 3.10)

project("machina")

find_package(fmt CONFIG REQUIRED)
find_package(raylib CONFIG REQUIRED)

add_executable(machina "src/main.cpp")

target_link_libraries(machina PRIVATE fmt::fmt)
target_link_libraries(machina PRIVATE raylib)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET machina PROPERTY CXX_STANDARD 20)
endif()