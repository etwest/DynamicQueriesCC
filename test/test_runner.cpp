#include <mpi.h>
#include <gtest/gtest.h>
#include "util.h"

static const std::string fname = __FILE__;
static size_t pos = fname.find_last_of("\\/");
static const std::string curr_dir = (std::string::npos == pos) ? "" : fname.substr(0, pos);
std::string stream_file = curr_dir + "/test_streams/multiples_graph_1024_stream.data";

int main(int argc, char** argv) {
  if (argc > 1)
    stream_file = argv[1];
  testing::InitGoogleTest(&argc, argv);
  int ret = RUN_ALL_TESTS(); 
  return ret; 
}
