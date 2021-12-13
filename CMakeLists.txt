cmake_minimum_required(VERSION 3.15)
include (ExternalProject)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

project(StreamingCCExperiments)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  message("Adding GNU compiler flags")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  message("Adding MSVC compiler flags")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall")
else()
  message("${CMAKE_CXX_COMPILER_ID} not recognized, no flags added")
endif()

message("${CMAKE_MODULE_PATH}")
include("DownloadProject.cmake")

# Install GraphStreamingCC Project
ExternalProject_Add(
  GraphStreamingCC

  PREFIX          GraphStreamingCC
  GIT_REPOSITORY  "https://github.com/GraphStreamingProject/GraphStreamingCC"
  GIT_TAG         "main"

  UPDATE_DISCONNECTED false
  BUILD_ALWAYS    OFF
  INSTALL_DIR     ${CMAKE_CURRENT_BINARY_DIR}/GraphStreamingCC/prefix

  CMAKE_CACHE_ARGS
        -DAUTODOWNLOAD:BOOL=ON
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>

  BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target install
)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/GraphStreamingCC/prefix/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/GraphStreamingCC/prefix/include/GutterTree)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/GraphStreamingCC/prefix/include/test)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/GraphStreamingCC/prefix/include/GraphZeppelinCommon)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/GraphStreamingCC/prefix/lib)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/GraphStreamingCC/src/GraphStreamingCC-build/GutterTree/prefix/lib)

add_executable(tests
  test/test_runner.cpp
  test/basic_test.cpp
  include/euler_tour_trees.h
  include/graph_tiers.h
  src/euler_tour_trees.cpp
  src/graph_tiers.cpp
)
add_dependencies(tests GraphStreamingCC)
target_link_libraries(tests PRIVATE libGraphStreamingCC.a GTest::gtest libGutterTree.a)
# optimize unless DEBUG
if (DEFINED ENV{DEBUG})
  message("Disabling optimizations and enabling debug symbols")
  target_compile_options(tests PRIVATE -g -fopenmp)
  target_link_options(tests PRIVATE -fopenmp)
else ()
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(tests PRIVATE -O3 -fopenmp)
    target_link_options(tests PRIVATE -fopenmp)
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    target_compile_options(tests PRIVATE /O2)
  endif()
endif()
