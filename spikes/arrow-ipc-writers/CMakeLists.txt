cmake_minimum_required(VERSION 3.20)
project(haybarn_arrow_ipc_writer_spike LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(HAYBARN_NODE_NEO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(HAYBARN_DIST_DIR "${HAYBARN_NODE_NEO_ROOT}/bindings/libhaybarn" CACHE PATH
    "Directory containing duckdb.h and the HayBarn shared library")

option(SPIKE_BUILD_NANOARROW "Build the nanoarrow comparison writer" ON)
option(SPIKE_BUILD_ARROW_CPP "Build the Arrow C++ comparison writers" ON)

if(SPIKE_BUILD_NANOARROW)
  include(FetchContent)

  # Pin the exact nanoarrow revision used by the experiment. This is
  # intentionally not added to the production binding until reviewed.
  set(NANOARROW_IPC ON CACHE BOOL "" FORCE)
  set(NANOARROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(NANOARROW_BUILD_INTEGRATION_TESTS OFF CACHE BOOL "" FORCE)
  set(NANOARROW_INSTALL_SHARED OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    nanoarrow
    GIT_REPOSITORY https://github.com/Query-farm/arrow-nanoarrow.git
    GIT_TAG de091e0a7d2d8906457ce71d4042fc9111d94c00
  )
  FetchContent_MakeAvailable(nanoarrow)
endif()

if(SPIKE_BUILD_ARROW_CPP)
  find_package(Arrow CONFIG REQUIRED)
endif()

if(APPLE)
  set(HAYBARN_LIBRARY "${HAYBARN_DIST_DIR}/libhaybarn.dylib")
elseif(WIN32)
  set(HAYBARN_LIBRARY "${HAYBARN_DIST_DIR}/haybarn.lib")
else()
  set(HAYBARN_LIBRARY "${HAYBARN_DIST_DIR}/libhaybarn.so")
endif()

add_library(haybarn_spike_common INTERFACE)
target_include_directories(haybarn_spike_common INTERFACE
  "${HAYBARN_DIST_DIR}"
  "${CMAKE_CURRENT_LIST_DIR}"
)
target_link_libraries(haybarn_spike_common INTERFACE "${HAYBARN_LIBRARY}")

if(SPIKE_BUILD_NANOARROW)
  add_executable(haybarn_ipc_nanoarrow nanoarrow_writer.cpp)
  target_link_libraries(haybarn_ipc_nanoarrow PRIVATE
    haybarn_spike_common
    nanoarrow_ipc_static
  )
endif()

if(SPIKE_BUILD_ARROW_CPP)
  if(TARGET Arrow::arrow_shared)
    add_executable(haybarn_ipc_arrow_cpp arrow_cpp_writer.cpp)
    target_link_libraries(haybarn_ipc_arrow_cpp PRIVATE
      haybarn_spike_common
      Arrow::arrow_shared
    )
  endif()

  add_executable(haybarn_ipc_arrow_cpp_static arrow_cpp_writer.cpp)
  target_link_libraries(haybarn_ipc_arrow_cpp_static PRIVATE
    haybarn_spike_common
    Arrow::arrow_static
  )
endif()

if(APPLE)
  set(spike_targets)
  if(TARGET haybarn_ipc_nanoarrow)
    list(APPEND spike_targets haybarn_ipc_nanoarrow)
  endif()
  if(TARGET haybarn_ipc_arrow_cpp_static)
    list(APPEND spike_targets haybarn_ipc_arrow_cpp_static)
  endif()
  if(TARGET haybarn_ipc_arrow_cpp)
    list(APPEND spike_targets haybarn_ipc_arrow_cpp)
  endif()
  foreach(target ${spike_targets})
    set_target_properties(${target} PROPERTIES
      BUILD_RPATH "${HAYBARN_DIST_DIR}"
    )
  endforeach()
elseif(UNIX)
  set(spike_targets)
  if(TARGET haybarn_ipc_nanoarrow)
    list(APPEND spike_targets haybarn_ipc_nanoarrow)
  endif()
  if(TARGET haybarn_ipc_arrow_cpp_static)
    list(APPEND spike_targets haybarn_ipc_arrow_cpp_static)
  endif()
  if(TARGET haybarn_ipc_arrow_cpp)
    list(APPEND spike_targets haybarn_ipc_arrow_cpp)
  endif()
  foreach(target ${spike_targets})
    set_target_properties(${target} PROPERTIES
      BUILD_RPATH "${HAYBARN_DIST_DIR}"
    )
  endforeach()
endif()
