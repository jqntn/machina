set(
    MACHINA_NVIDIA_USD_URL
    "https://developer.nvidia.com/downloads/usd/usd_binaries/25.08/usd.py312.windows-x86_64.usdview.release-v25.08.71e038c1.zip")
set(MACHINA_NVIDIA_USD_SHA256 "61bae28d18c873871047e7a8b3fe1ffe2188fb88fdde113be429812d27f0c8b4")
set(MACHINA_NVIDIA_USD_ARCHIVE_NAME "usd.py312.windows-x86_64.usdview.release-v25.08.71e038c1.zip")
set(MACHINA_NVIDIA_USD_VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/out/vendor")
set(MACHINA_NVIDIA_USD_ROOT "${MACHINA_NVIDIA_USD_VENDOR_DIR}/nvidia-usd-25.08")
set(MACHINA_NVIDIA_USD_ARCHIVE "${MACHINA_NVIDIA_USD_VENDOR_DIR}/${MACHINA_NVIDIA_USD_ARCHIVE_NAME}")
set(MACHINA_NVIDIA_USD_STAMP "${MACHINA_NVIDIA_USD_ROOT}/.machina_nvidia_usd_25.08.stamp")

set(MACHINA_NVIDIA_USD_LIBRARY_NAMES
    usd_usdMtlx
    usd_usdUI
    usd_usdUtils
    usd_usdShade
    usd_usdGeom
    usd_usd
    usd_kind
    usd_pcp
    usd_sdr
    usd_sdf
    usd_ar
    usd_plug
    usd_ts
    usd_vt
    usd_work
    usd_trace
    usd_js
    usd_pegtl
    usd_gf
    usd_tf
    usd_arch
    usd_python
    usd_boost
    MaterialXGenGlsl
    MaterialXGenShader
    MaterialXFormat
    MaterialXCore
    tbb)

function(machina_nvidia_usd_lib output_variable library_name)
    set("${output_variable}" "${MACHINA_NVIDIA_USD_ROOT}/lib/${library_name}.lib" PARENT_SCOPE)
endfunction()

function(machina_ensure_nvidia_usd_sdk)
    message(STATUS "Ensuring NVIDIA OpenUSD 25.08 SDK")
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}"
            "-DMACHINA_NVIDIA_USD_URL=${MACHINA_NVIDIA_USD_URL}"
            "-DMACHINA_NVIDIA_USD_SHA256=${MACHINA_NVIDIA_USD_SHA256}"
            "-DMACHINA_NVIDIA_USD_ARCHIVE=${MACHINA_NVIDIA_USD_ARCHIVE}"
            "-DMACHINA_NVIDIA_USD_ROOT=${MACHINA_NVIDIA_USD_ROOT}"
            "-DMACHINA_NVIDIA_USD_STAMP=${MACHINA_NVIDIA_USD_STAMP}"
            -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/DownloadNvidiaUsd.cmake"
        COMMAND_ERROR_IS_FATAL ANY)
endfunction()

function(machina_add_nvidia_usd_sdk)
    machina_ensure_nvidia_usd_sdk()

    set(nvidia_usd_link_libraries)
    foreach(library_name IN LISTS MACHINA_NVIDIA_USD_LIBRARY_NAMES)
        machina_nvidia_usd_lib(library_path "${library_name}")
        list(APPEND nvidia_usd_link_libraries "${library_path}")
    endforeach()
    list(APPEND nvidia_usd_link_libraries "${MACHINA_NVIDIA_USD_ROOT}/python/libs/python312.lib")

    add_library(machina_nvidia_usd INTERFACE)

    target_compile_definitions(
        machina_nvidia_usd
        INTERFACE NOMINMAX
                  BOOST_ALL_NO_LIB
                  PXR_BOOST_PYTHON_NO_PY_SIGNATURES
                  Py_NO_LINK_LIB
                  _ITERATOR_DEBUG_LEVEL=0
                  __TBB_NO_IMPLICIT_LINKAGE=1
                  __TBBMALLOC_NO_IMPLICIT_LINKAGE=1)
    target_include_directories(
        machina_nvidia_usd
        INTERFACE "${MACHINA_NVIDIA_USD_ROOT}/include"
                  "${MACHINA_NVIDIA_USD_ROOT}/python/include")
    target_link_libraries(
        machina_nvidia_usd
        INTERFACE ${nvidia_usd_link_libraries}
                  Ws2_32.lib
                  Dbghelp.lib
                  Shlwapi.lib)

    set(MACHINA_MATERIALX_LIBRARY_RUNTIME_ROOT "materialx" PARENT_SCOPE)
endfunction()

function(machina_copy_nvidia_usd_runtime target)
    add_custom_command(
        TARGET "${target}"
        POST_BUILD
        COMMAND
            "${CMAKE_COMMAND}"
            "-DMACHINA_NVIDIA_USD_ROOT=${MACHINA_NVIDIA_USD_ROOT}"
            "-DMACHINA_NVIDIA_USD_RUNTIME_DIR=$<TARGET_FILE_DIR:${target}>"
            "-DMACHINA_MATERIALX_LIBRARY_RUNTIME_ROOT=${MACHINA_MATERIALX_LIBRARY_RUNTIME_ROOT}"
            -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/CopyNvidiaUsdRuntime.cmake")
endfunction()
