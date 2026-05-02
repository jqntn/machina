cmake_minimum_required(VERSION 3.15)

project(machina LANGUAGES CXX)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

find_package(fmt CONFIG REQUIRED)

FetchContent_Declare(
  raylib
  URL "https://github.com/raysan5/raylib/archive/cf9f27db545dd686b03f9f773cfbdb8a8dd9f72f.tar.gz"
  URL_HASH SHA256=2ebe3e90cd66b16233a370406b7abc76392d45713a4302f14a68e87e129090a1
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(machina)

set_property(TARGET machina PROPERTY CXX_STANDARD 20)
set_property(TARGET machina PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET machina PROPERTY CXX_EXTENSIONS ON)

file(GLOB_RECURSE src "src/*.cpp")
target_sources(machina PRIVATE "${src}")

target_include_directories(machina PRIVATE "include")

target_link_libraries(machina PRIVATE fmt::fmt)
target_link_libraries(machina PRIVATE raylib)