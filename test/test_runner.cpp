#include <gtest/gtest.h>


std::string stream_file;

int main(int argc, char** argv) {
  stream_file = argv[1];
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
