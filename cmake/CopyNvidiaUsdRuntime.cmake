foreach(required MACHINA_NVIDIA_USD_ROOT MACHINA_NVIDIA_USD_RUNTIME_DIR MACHINA_MATERIALX_LIBRARY_RUNTIME_ROOT)
    if(NOT DEFINED "${required}" OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "${required} is required")
    endif()
endforeach()

foreach(required_path bin lib python libraries plugin/usd lib/usd)
    if(NOT EXISTS "${MACHINA_NVIDIA_USD_ROOT}/${required_path}")
        message(FATAL_ERROR "NVIDIA OpenUSD runtime path is missing: ${MACHINA_NVIDIA_USD_ROOT}/${required_path}")
    endif()
endforeach()

file(GLOB nvidia_usd_dlls
     "${MACHINA_NVIDIA_USD_ROOT}/bin/*.dll"
     "${MACHINA_NVIDIA_USD_ROOT}/lib/*.dll"
     "${MACHINA_NVIDIA_USD_ROOT}/python/*.dll")
if(nvidia_usd_dlls)
    file(COPY ${nvidia_usd_dlls} DESTINATION "${MACHINA_NVIDIA_USD_RUNTIME_DIR}")
endif()

file(REMOVE_RECURSE "${MACHINA_NVIDIA_USD_RUNTIME_DIR}/usd")
file(REMOVE_RECURSE "${MACHINA_NVIDIA_USD_RUNTIME_DIR}/usd_plugins")
file(REMOVE_RECURSE "${MACHINA_NVIDIA_USD_RUNTIME_DIR}/${MACHINA_MATERIALX_LIBRARY_RUNTIME_ROOT}/libraries")

file(COPY "${MACHINA_NVIDIA_USD_ROOT}/lib/usd/" DESTINATION "${MACHINA_NVIDIA_USD_RUNTIME_DIR}/usd")
file(COPY "${MACHINA_NVIDIA_USD_ROOT}/plugin/usd/" DESTINATION "${MACHINA_NVIDIA_USD_RUNTIME_DIR}/usd_plugins")
file(
    COPY "${MACHINA_NVIDIA_USD_ROOT}/libraries"
    DESTINATION "${MACHINA_NVIDIA_USD_RUNTIME_DIR}/${MACHINA_MATERIALX_LIBRARY_RUNTIME_ROOT}")
