foreach(required MACHINA_NVIDIA_USD_URL MACHINA_NVIDIA_USD_SHA256 MACHINA_NVIDIA_USD_ARCHIVE MACHINA_NVIDIA_USD_ROOT MACHINA_NVIDIA_USD_STAMP)
    if(NOT DEFINED "${required}" OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "${required} is required")
    endif()
endforeach()

set(required_nvidia_usd_paths
    "${MACHINA_NVIDIA_USD_ROOT}/include/pxr/usd/usd/stage.h"
    "${MACHINA_NVIDIA_USD_ROOT}/include/MaterialXGenGlsl/GlslShaderGenerator.h"
    "${MACHINA_NVIDIA_USD_ROOT}/lib/usd_usd.lib"
    "${MACHINA_NVIDIA_USD_ROOT}/lib/MaterialXGenGlsl.lib"
    "${MACHINA_NVIDIA_USD_ROOT}/python/libs/python312.lib")

set(nvidia_usd_ready TRUE)
foreach(required_path IN LISTS required_nvidia_usd_paths)
    if(NOT EXISTS "${required_path}")
        set(nvidia_usd_ready FALSE)
    endif()
endforeach()

if(EXISTS "${MACHINA_NVIDIA_USD_STAMP}" AND nvidia_usd_ready)
    return()
endif()

get_filename_component(archive_directory "${MACHINA_NVIDIA_USD_ARCHIVE}" DIRECTORY)
file(MAKE_DIRECTORY "${archive_directory}")

if(NOT EXISTS "${MACHINA_NVIDIA_USD_ARCHIVE}")
    set(temporary_archive "${MACHINA_NVIDIA_USD_ARCHIVE}.tmp")
    file(REMOVE "${temporary_archive}")
    file(
        DOWNLOAD "${MACHINA_NVIDIA_USD_URL}" "${temporary_archive}"
        STATUS download_status
        EXPECTED_HASH "SHA256=${MACHINA_NVIDIA_USD_SHA256}"
        SHOW_PROGRESS
        TLS_VERIFY ON)
    list(GET download_status 0 download_status_code)
    list(GET download_status 1 download_status_message)
    if(NOT download_status_code EQUAL 0)
        file(REMOVE "${temporary_archive}")
        message(FATAL_ERROR "NVIDIA OpenUSD download failed: ${download_status_message}")
    endif()
    file(RENAME "${temporary_archive}" "${MACHINA_NVIDIA_USD_ARCHIVE}")
endif()

file(REMOVE_RECURSE "${MACHINA_NVIDIA_USD_ROOT}")
file(MAKE_DIRECTORY "${MACHINA_NVIDIA_USD_ROOT}")
file(ARCHIVE_EXTRACT INPUT "${MACHINA_NVIDIA_USD_ARCHIVE}" DESTINATION "${MACHINA_NVIDIA_USD_ROOT}")
file(REMOVE "${MACHINA_NVIDIA_USD_ARCHIVE}")
file(WRITE "${MACHINA_NVIDIA_USD_STAMP}" "${MACHINA_NVIDIA_USD_URL}\n")
