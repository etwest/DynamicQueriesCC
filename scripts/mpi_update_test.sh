#!/bin/bash

declare base_dir="$(dirname $(dirname $(realpath $0)))"

cd ${base_dir}/build
set -e
make -j
set +e

mkdir -p ./../results

# DEFAULT BATCH SIZE (100), DEFAULT SKIPLIST HEIGHT FACTOR (1 / log log n) ==============================================================

# mpirun -np 23 ./mpi_dynamicCC_tests binary_streams/kron_13_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 26 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_15_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_16_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 30 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_17_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 31 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_18_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*

mpirun -np 19 ./mpi_dynamicCC_tests binary_streams/dnc_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 26 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/tech_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 29 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/enron_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 19 ./mpi_dynamicCC_tests binary_streams/dnc_streamified_binary 0 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 26 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/tech_streamified_binary 0 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 29 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/enron_streamified_binary 0 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 19 ./mpi_dynamicCC_tests binary_streams/dnc_ff_binary 0 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 26 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/tech_ff_binary 0 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 29 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/enron_ff_binary 0 0 --gtest_filter=*mpi_update_speed_test*

mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/twitter_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 31 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/stanford_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 32 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/random2N_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 29 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/randomNLOGN_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 25 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/randomNSQRTN_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 29 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/randomDIV_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*


# BATCH SIZE = 1, DEFAULT SKIPLIST HEIGHT FACTOR (1 / log log n) =======================================================================

# mpirun -np 23 ./mpi_dynamicCC_tests binary_streams/kron_13_stream_binary 1 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 26 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_15_stream_binary 1 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_16_stream_binary 1 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 30 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_17_stream_binary 1 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 31 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_18_stream_binary 1 0 --gtest_filter=*mpi_update_speed_test*

mpirun -np 19 ./mpi_dynamicCC_tests binary_streams/dnc_stream_binary 1 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 26 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/tech_stream_binary 1 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 29 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/enron_stream_binary 1 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 19 ./mpi_dynamicCC_tests binary_streams/dnc_streamified_binary 1 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 26 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/tech_streamified_binary 1 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 29 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/enron_streamified_binary 1 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 19 ./mpi_dynamicCC_tests binary_streams/dnc_ff_binary 1 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 26 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/tech_ff_binary 1 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 29 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/enron_ff_binary 1 0 --gtest_filter=*mpi_update_speed_test*

mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/twitter_stream_binary 1 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 31 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/stanford_stream_binary 1 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 32 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/random2N_stream_binary 1 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 29 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/randomNLOGN_stream_binary 1 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 25 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/randomNSQRTN_stream_binary 1 0 --gtest_filter=*mpi_update_speed_test*
mpirun -np 29 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/randomDIV_stream_binary 1 0 --gtest_filter=*mpi_update_speed_test*



# These are long so may want to do them last
# mpirun -np 30 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_17_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 31 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_18_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 30 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_17_stream_binary 1 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 31 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_18_stream_binary 1 0 --gtest_filter=*mpi_update_speed_test*
