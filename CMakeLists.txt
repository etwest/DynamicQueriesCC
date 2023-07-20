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

######
# Get MPI for distributed communication
######
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
add_definitions(-DOMPI_SKIP_MPICXX)
if(MPI_CXX_FOUND)
       message("Found MPI_CXX")
endif()



# Install GraphZeppelin Project
FetchContent_Declare(
  GraphZeppelinVerifyCC

  GIT_REPOSITORY      https://github.com/GraphStreamingProject/GraphZeppelin
  GIT_TAG             main
)

FetchContent_MakeAvailable(GraphZeppelinVerifyCC)

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)
#add_compile_options(-fsanitize=undefined)
#add_link_options(-fsanitize=undefined)

add_executable(dynamicCC_tests
  test/test_runner.cpp
  test/skiplist_test.cpp
  test/euler_tour_tree_test.cpp
  test/link_cut_tree_test.cpp
  test/graph_tiers_test.cpp

  src/sketch.cpp

  src/skiplist.cpp
  src/euler_tour_tree.cpp
  src/link_cut_tree.cpp

  src/input_node.cpp
  src/tier_node.cpp
)

target_include_directories(dynamicCC_tests PUBLIC include ${MPI_C_INCLUDE_PATH})
add_dependencies(dynamicCC_tests GraphZeppelinVerifyCC)
target_link_libraries(dynamicCC_tests PRIVATE GraphZeppelinVerifyCC ${MPI_LIBRARIES})

#######
# TODO: Is MPI INCLUDE PATH necessary?
if(MPI_COMPILE_FLAGS)
  set_target_properties(dynamicCC_tests PROPERTIES
    COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()
if(MPI_LINK_FLAGS)
  set_target_properties(dynamicCC_tests PROPERTIES
    LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()

#######
