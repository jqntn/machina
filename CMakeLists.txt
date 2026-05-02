cmake_minimum_required(VERSION 3.20)

project(machina LANGUAGES CXX)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    raylib
    URL "https://github.com/raysan5/raylib/archive/cf9f27db545dd686b03f9f773cfbdb8a8dd9f72f.tar.gz"
    URL_HASH SHA256=2ebe3e90cd66b16233a370406b7abc76392d45713a4302f14a68e87e129090a1
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(raylib)

find_package(EnTT CONFIG REQUIRED)
find_package(MaterialX CONFIG REQUIRED)
find_package(pxr CONFIG REQUIRED)

set(MACHINA_ENGINE_SOURCES
    src/LevelInstantiator.cpp
    src/MaterialXShaderGenerator.cpp
    src/Renderer.cpp
    src/UsdLevelLoader.cpp)

file(TO_CMAKE_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}" MACHINA_VCPKG_PREFIX)

set(MACHINA_ASSETS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(MACHINA_ASSETS_RUNTIME_ROOT "assets")

set(MACHINA_MATERIALX_LIBRARY_SOURCE_DIR "${MACHINA_VCPKG_PREFIX}/libraries")
set(MACHINA_MATERIALX_LIBRARY_RUNTIME_ROOT "materialx")

set(MACHINA_VCPKG_RUNTIME_BIN_DIR "${MACHINA_VCPKG_PREFIX}/bin")
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND EXISTS "${MACHINA_VCPKG_PREFIX}/debug/bin")
    set(MACHINA_VCPKG_RUNTIME_BIN_DIR "${MACHINA_VCPKG_PREFIX}/debug/bin")
endif()

set(MACHINA_USD_PLUGIN_SOURCE_DIR "${MACHINA_VCPKG_PREFIX}/bin/usd")
set(MACHINA_USD_PLUGIN_RUNTIME_DIR "${CMAKE_CURRENT_BINARY_DIR}/usd_plugins")
set(MACHINA_USD_PLUGIN_INDEX "${CMAKE_CURRENT_BINARY_DIR}/usd")
file(GLOB MACHINA_USD_RUNTIME_DLLS CONFIGURE_DEPENDS "${MACHINA_VCPKG_RUNTIME_BIN_DIR}/usd_*.dll")

if(EXISTS "${MACHINA_USD_PLUGIN_SOURCE_DIR}")
    file(REMOVE_RECURSE "${MACHINA_USD_PLUGIN_RUNTIME_DIR}")
    file(REMOVE_RECURSE "${MACHINA_USD_PLUGIN_INDEX}")
    file(COPY "${MACHINA_USD_PLUGIN_SOURCE_DIR}/" DESTINATION "${MACHINA_USD_PLUGIN_RUNTIME_DIR}")
    file(
        WRITE "${MACHINA_USD_PLUGIN_INDEX}"
        "{\n  \"Includes\": [\n    \"usd_plugins/**/\"\n  ]\n}\n")
endif()

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
           usd
           usdGeom
           usdShade
           usdMtlx
           MaterialXCore
           MaterialXFormat
           MaterialXGenShader
           MaterialXGenGlsl)

add_executable(machina src/main.cpp)
target_link_libraries(machina PRIVATE machina_engine)

find_program(
    CLANG_FORMAT_EXE
    NAMES clang-format clang-format.exe
    HINTS "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/bin")

if(NOT CLANG_FORMAT_EXE)
    message(FATAL_ERROR "clang-format is required for the format target")
endif()

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

add_test(NAME level_smoke COMMAND machina_level_smoke)
set_tests_properties(level_smoke PROPERTIES WORKING_DIRECTORY "$<TARGET_FILE_DIR:machina_level_smoke>")

function(machina_copy_materialx_libraries target)
    add_custom_command(
        TARGET "${target}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
                "$<TARGET_FILE_DIR:${target}>/${MACHINA_MATERIALX_LIBRARY_RUNTIME_ROOT}"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${MACHINA_MATERIALX_LIBRARY_SOURCE_DIR}"
                "$<TARGET_FILE_DIR:${target}>/${MACHINA_MATERIALX_LIBRARY_RUNTIME_ROOT}/libraries")
endfunction()

function(machina_copy_assets target)
    add_custom_command(
        TARGET "${target}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${MACHINA_ASSETS_SOURCE_DIR}"
                "$<TARGET_FILE_DIR:${target}>/${MACHINA_ASSETS_RUNTIME_ROOT}")
endfunction()

function(machina_copy_usd_runtime target)
    if(MACHINA_USD_RUNTIME_DLLS)
        add_custom_command(
            TARGET "${target}"
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${MACHINA_USD_RUNTIME_DLLS}
                    "$<TARGET_FILE_DIR:${target}>")
    endif()
endfunction()

machina_copy_materialx_libraries(machina)
machina_copy_materialx_libraries(machina_level_smoke)
machina_copy_assets(machina)
machina_copy_assets(machina_level_smoke)
machina_copy_usd_runtime(machina)
machina_copy_usd_runtime(machina_level_smoke)

include(cmake/MakeDistribution.cmake)

machina_add_distribution_target(
    TARGET machina
    OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/out/distrib")
