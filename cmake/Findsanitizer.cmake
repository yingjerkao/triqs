# Copyright Nils Wentzell 2018
# Distributed under the GNU GENERAL PUBLIC LICENSE Version 3.0.
# See accompanying file LICENSE or https://www.gnu.org/licenses/gpl-3.0.txt
#
# This cmake find module looks for the LLVM Sanitizer Runtime Librariies
# It sets up SANITIZER_RT_LIBRARIES and ${COMPONENT}_RT_LIBRARY for each
# requested component.
#
# Use this module by invoking find_package with the form::
#
#   find_package(sanitizer [REQUIRED] [asan] [ubsan])
#
# Results are reported in::
#
#    SANITIZER_RT_LIBRARIES 		All Sanitizer Runtime Libraries
#    ${COMPONENT}_RT_LIBRARY 		Individual Sanitizer Runtime Library

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-resource-dir
    OUTPUT_VARIABLE clang_resource_dir OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(prefix clang_rt.)
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(search_path ${clang_resource_dir}/lib/darwin)
    set(suffix _osx_dynamic)
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(search_path ${clang_resource_dir}/lib/linux)
    set(suffix -${CMAKE_SYSTEM_PROCESSOR})
  else()
    message(FATAL_ERROR "Unknown platform")
  endif()
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-libgcc-file-name
    OUTPUT_VARIABLE libgcc_file OUTPUT_STRIP_TRAILING_WHITESPACE)
  get_filename_component(search_path ${libgcc_file} DIRECTORY)
else()
  message(FATAL_ERROR "Sanitizer is not available for your compiler")
endif()

set(SANITIZER_RT_LIBRARIES "")
set(required_vars SANITIZER_RT_LIBRARIES)
foreach(component ${${CMAKE_FIND_PACKAGE_NAME}_FIND_COMPONENTS})
  string(TOUPPER ${component} COMPONENT)
  if((${component} STREQUAL "ubsan") AND (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang") AND ASAN_RT_LIBRARY)
    set(component ubsan_minimal)
  endif()

  find_library(${COMPONENT}_RT_LIBRARY
    NAMES ${prefix}${component}${suffix} ${prefix}${component}_standalone${suffix}
    PATHS ${search_path})
  mark_as_advanced(${COMPONENT}_RT_LIBRARY)

  # Imported target
  add_library(lib${component}_rt SHARED IMPORTED)
  set_property(TARGET lib${component}_rt PROPERTY IMPORTED_LOCATION ${${COMPONENT}_RT_LIBRARY})

  if(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED_${component})
    list(APPEND required_vars ${COMPONENT}_RT_LIBRARY)
  endif()
  list(APPEND SANITIZER_RT_LIBRARIES ${${COMPONENT}_RT_LIBRARY})
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args("Sanitizer Runtime Libraries"
  REQUIRED_VARS ${required_vars}
  FAIL_MESSAGE "Sanitizer Runtime Libraries not found! Consider installing for additional checks!"
)
