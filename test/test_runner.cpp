#include <mpi.h>
#include <gtest/gtest.h>
#include "util.h"


std::string stream_file;

int main(int argc, char** argv) {
  if (argc > 1)
    stream_file = argv[1];
  testing::InitGoogleTest(&argc, argv);
  int ret = RUN_ALL_TESTS(); 
  return ret; 
}
