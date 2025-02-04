#include <mpi.h>
#include <gtest/gtest.h>
#include "util.h"

static const std::string fname = __FILE__;
static size_t pos = fname.find_last_of("\\/");
static const std::string curr_dir = (std::string::npos == pos) ? "" : fname.substr(0, pos);

std::string stream_file = curr_dir + "/test_streams/multiples_graph_1024_stream.data";
int batch_size_arg = 4;
double height_factor_arg = 1 / 10;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  
  if (argc != 4 && argc != 1) {
    std::cerr << "INCORRECT NUMBER OF ARGUMENTS." << std::endl;
    std::cerr << "Expect: stream_file, batch_size_arg, height_factor_arg" << std::endl;
    std::cerr << "OR: Run without any arguments to use default values of all for unit testing" << std::endl;
    return EXIT_FAILURE;
  }
  if (argc == 1) {
    std::cerr << "RUNNING WITH DEFAULT ARGUMENTS" << std::endl;
    std::cerr << "stream_file       = " << stream_file << std::endl; 
    std::cerr << "batch_size_arg    = " << batch_size_arg << std::endl;
    std::cerr << "height_factor_arg = " << height_factor_arg << std::endl;
  } else {
    stream_file = argv[1];
    batch_size_arg = atoi(argv[2]);
    height_factor_arg = atof(argv[3]);
  }

  testing::InitGoogleTest(&argc, argv);
  int ret = RUN_ALL_TESTS(); 
  MPI_Finalize();
  return ret; 
}

