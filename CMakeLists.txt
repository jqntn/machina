cmake_minimum_required(VERSION 3.20)

project(machina LANGUAGES CXX)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    raylib
    URL "https://github.com/raysan5/raylib/archive/cf9f27db545dd686b03f9f773cfbdb8a8dd9f72f.tar.gz"
    URL_HASH SHA256=2ebe3e90cd66b16233a370406b7abc76392d45713a4302f14a68e87e129090a1
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(raylib)

find_package(EnTT CONFIG REQUIRED)
include(cmake/NvidiaUsd.cmake)
machina_add_nvidia_usd_sdk()

set(MACHINA_ENGINE_SOURCES
    src/LevelInstantiator.cpp
    src/MaterialXShaderGenerator.cpp
    src/Renderer.cpp
    src/UsdLevelLoader.cpp)

set(MACHINA_ASSETS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(MACHINA_ASSETS_RUNTIME_ROOT "assets")

add_library(machina_engine STATIC ${MACHINA_ENGINE_SOURCES})
target_compile_features(machina_engine PUBLIC cxx_std_23)
target_include_directories(machina_engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(
    machina_engine
    PUBLIC MACHINA_ASSETS_ROOT="${MACHINA_ASSETS_RUNTIME_ROOT}"
           MACHINA_MATERIALX_LIBRARY_ROOT="${MACHINA_MATERIALX_LIBRARY_RUNTIME_ROOT}")
target_link_libraries(
    machina_engine
    PUBLIC raylib
           EnTT::EnTT
           machina_nvidia_usd)

add_executable(machina src/main.cpp)
target_link_libraries(machina PRIVATE machina_engine)

find_program(
    CLANG_FORMAT_EXE
    NAMES clang-format clang-format.exe
    HINTS "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/bin")

file(
    GLOB_RECURSE
    MACHINA_FORMAT_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")

add_custom_target(
    format
    COMMAND "${CLANG_FORMAT_EXE}" -i ${MACHINA_FORMAT_SOURCES}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Formatting machina sources")

add_dependencies(machina_engine format)
add_dependencies(machina format)

enable_testing()

add_executable(machina_level_smoke tests/level_smoke.cpp)
target_link_libraries(machina_level_smoke PRIVATE machina_engine)
add_dependencies(machina_level_smoke machina)

add_test(NAME level_smoke COMMAND machina_level_smoke)
set_tests_properties(level_smoke PROPERTIES WORKING_DIRECTORY "$<TARGET_FILE_DIR:machina_level_smoke>")

function(machina_copy_assets target)
    add_custom_command(
        TARGET "${target}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${MACHINA_ASSETS_SOURCE_DIR}"
                "$<TARGET_FILE_DIR:${target}>/${MACHINA_ASSETS_RUNTIME_ROOT}")
endfunction()

machina_copy_assets(machina)
machina_copy_nvidia_usd_runtime(machina)

include(cmake/MakeDistribution.cmake)

machina_add_distribution_target(
    TARGET machina
    OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/out/distrib")
