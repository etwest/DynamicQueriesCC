#include <mpi.h>
#include <gtest/gtest.h>
#include "util.h"


std::string stream_file;
int hybrid_threshold_arg;
int batch_size_arg;
double height_factor_arg;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  
  if (argc < 5) {
    std::cerr << "INCORRECT NUMBER OF ARGUMENTS." << std::endl;
    return EXIT_FAILURE;
  }

  stream_file = argv[1];
  batch_size_arg = atoi(argv[2]);
  height_factor_arg = atof(argv[3]);
  hybrid_threshold_arg = atoi(argv[4]);

  testing::InitGoogleTest(&argc, argv);
  int ret = RUN_ALL_TESTS(); 
  MPI_Finalize();
  return ret; 
}
