#!/bin/bash

declare base_dir="$(dirname $(dirname $(realpath $0)))"
echo "Testing with buffer size $1"

cd ${base_dir}/build
set -e
cmake -DSKETCH_BUFFER_SIZE=$1 ..
make -j
set +e

mkdir -p ./../results
mkdir -p ./../results/mpi_speed_results

# Test run
# mpirun -np 23 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_13_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*

# KRON-16 Batch Size Sweep
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_16_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
# KRON-16 fixed-forest
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_16_ff_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*

# Twitter Batch Size Sweep
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/twitter_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
# Twitter fixed-forest
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/twitter_ff_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*

