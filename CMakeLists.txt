cmake_minimum_required(VERSION 3.15)
project(DynamicQueriesCC)
include (FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Make the default build type Release. If user or another
# project sets a different value than use that
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to default -- Release")
  set(CMAKE_BUILD_TYPE "Release" CACHE
      STRING "Choose the type of build." FORCE)
endif()
message(STATUS "DynamicQueries Build Type: ${CMAKE_BUILD_TYPE}")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  message("Adding GNU compiler flags")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  message("Adding MSVC compiler flags")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall")
else()
  message("${CMAKE_CXX_COMPILER_ID} not recognized, no flags added")
endif()

# Install GraphStreamingCC Project
FetchContent_Declare(
  GraphStreamingCC
  GIT_REPOSITORY      "https://github.com/GraphStreamingProject/GraphStreamingCC"
  GIT_TAG             "main"
  UPDATE_DISCONNECTED OFF

  CMAKE_CACHE_ARGS
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
)

FetchContent_MakeAvailable(GraphStreamingCC)

add_executable(dynamicCC_tests
  test/test_runner.cpp
  test/basic_test.cpp
  test/euler_tour_tree_test.cpp
  test/splay_tree_test.cpp

  src/splay_tree.cpp

  src/euler_tour_tree.cpp

  src/graph_tiers.cpp
)
target_include_directories(dynamicCC_tests PUBLIC include)
add_dependencies(dynamicCC_tests GraphStreamingCC)
target_link_libraries(dynamicCC_tests PRIVATE GraphStreamingCC)
