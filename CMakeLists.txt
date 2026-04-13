cmake_minimum_required(VERSION 3.10)

project("machina")

find_package(fmt CONFIG REQUIRED)

add_executable(machina "main.cpp")

target_link_libraries(machina PRIVATE fmt::fmt)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET machina PROPERTY CXX_STANDARD 20)
endif()