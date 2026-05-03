function(machina_msvc_redist_dlls output_variable)
  if(NOT DEFINED CMAKE_CXX_COMPILER OR CMAKE_CXX_COMPILER STREQUAL "")
    set("${output_variable}" "" PARENT_SCOPE)
    return()
  endif()

  if(NOT EXISTS "${CMAKE_CXX_COMPILER}")
    set("${output_variable}" "" PARENT_SCOPE)
    return()
  endif()

  get_filename_component(cl_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
  get_filename_component(vc_dir "${cl_dir}/../../../../../.." ABSOLUTE)
  set(msvc_redist_root "${vc_dir}/Redist/MSVC")

  file(
    GLOB
    msvc_crt_dirs
    LIST_DIRECTORIES true
    "${msvc_redist_root}/*/x64/Microsoft.VC*.CRT")

  if(NOT msvc_crt_dirs)
    set("${output_variable}" "" PARENT_SCOPE)
    return()
  endif()

  list(SORT msvc_crt_dirs COMPARE NATURAL)
  list(LENGTH msvc_crt_dirs msvc_crt_dir_count)
  math(EXPR msvc_crt_dir_last "${msvc_crt_dir_count} - 1")
  list(GET msvc_crt_dirs "${msvc_crt_dir_last}" msvc_crt_dir)

  file(GLOB msvc_redist_dlls "${msvc_crt_dir}/*.dll")
  set("${output_variable}" ${msvc_redist_dlls} PARENT_SCOPE)
endfunction()

function(machina_add_distribution_target)
  cmake_parse_arguments(
    MACHINA_DISTRIB
    ""
    "TARGET;OUTPUT_DIR;NVIDIA_USD_ROOT;MATERIALX_LIBRARY_RUNTIME_ROOT"
    ""
    ${ARGN})

  if(MACHINA_DISTRIB_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unexpected arguments: ${MACHINA_DISTRIB_UNPARSED_ARGUMENTS}")
  endif()

  foreach(required TARGET OUTPUT_DIR NVIDIA_USD_ROOT MATERIALX_LIBRARY_RUNTIME_ROOT)
    if(NOT MACHINA_DISTRIB_${required})
      message(FATAL_ERROR "machina_add_distribution_target requires ${required}")
    endif()
  endforeach()

  if(NOT TARGET "${MACHINA_DISTRIB_TARGET}")
    message(FATAL_ERROR "Distribution target '${MACHINA_DISTRIB_TARGET}' does not exist")
  endif()

  machina_msvc_redist_dlls(msvc_redist_dlls)

  add_custom_target(
    distrib
    COMMAND "${CMAKE_COMMAND}" -E remove_directory "${MACHINA_DISTRIB_OUTPUT_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${MACHINA_DISTRIB_OUTPUT_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:${MACHINA_DISTRIB_TARGET}>"
            "${MACHINA_DISTRIB_OUTPUT_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "$<TARGET_FILE_DIR:${MACHINA_DISTRIB_TARGET}>/assets"
            "${MACHINA_DISTRIB_OUTPUT_DIR}/assets"
    COMMAND
            "${CMAKE_COMMAND}"
            "-DMACHINA_NVIDIA_USD_ROOT=${MACHINA_DISTRIB_NVIDIA_USD_ROOT}"
            "-DMACHINA_NVIDIA_USD_RUNTIME_DIR=${MACHINA_DISTRIB_OUTPUT_DIR}"
            "-DMACHINA_MATERIALX_LIBRARY_RUNTIME_ROOT=${MACHINA_DISTRIB_MATERIALX_LIBRARY_RUNTIME_ROOT}"
            -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/CopyNvidiaUsdRuntime.cmake"
    DEPENDS "${MACHINA_DISTRIB_TARGET}"
    COMMENT "Assembling machina distribution")

  add_dependencies(distrib "${MACHINA_DISTRIB_TARGET}")

  if(msvc_redist_dlls)
    add_custom_command(
      TARGET distrib
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${msvc_redist_dlls}
              "${MACHINA_DISTRIB_OUTPUT_DIR}")
  endif()
endfunction()
