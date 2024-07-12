#!/bin/bash

declare base_dir="$(dirname $(dirname $(realpath $0)))"

cd ${base_dir}/build
set -e
make -j
set +e

mkdir -p ./../results

# Test run
# mpirun -np 23 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_13_query10_binary 100 0 --gtest_filter=*mpi_mixed_speed_test*

# KRON-16 Batch Size Sweep
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_16_query10_binary 1 0 --gtest_filter=*mpi_mixed_speed_test*
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_16_query10_binary 10 0 --gtest_filter=*mpi_mixed_speed_test*
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_16_query10_binary 50 0 --gtest_filter=*mpi_mixed_speed_test*
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_16_query10_binary 100 0 --gtest_filter=*mpi_mixed_speed_test*
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_16_query10_binary 1000 0 --gtest_filter=*mpi_mixed_speed_test*
# KRON-16 fixed-forest
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_16_ff_query10_binary 1 0 --gtest_filter=*mpi_mixed_speed_test*
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_16_ff_query10_binary 10 0 --gtest_filter=*mpi_mixed_speed_test*
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_16_ff_query10_binary 50 0 --gtest_filter=*mpi_mixed_speed_test*
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_16_ff_query10_binary 100 0 --gtest_filter=*mpi_mixed_speed_test*
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_16_ff_query10_binary 1000 0 --gtest_filter=*mpi_mixed_speed_test*

# Twitter Batch Size Sweep
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/twitter_query10_binary 1 0 --gtest_filter=*mpi_mixed_speed_test*
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/twitter_query10_binary 10 0 --gtest_filter=*mpi_mixed_speed_test*
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/twitter_query10_binary 50 0 --gtest_filter=*mpi_mixed_speed_test*
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/twitter_query10_binary 100 0 --gtest_filter=*mpi_mixed_speed_test*
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/twitter_query10_binary 1000 0 --gtest_filter=*mpi_mixed_speed_test*
# Twitter fixed-forest
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/twitter_ff_query10_binary 1 0 --gtest_filter=*mpi_mixed_speed_test*
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/twitter_ff_query10_binary 10 0 --gtest_filter=*mpi_mixed_speed_test*
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/twitter_ff_query10_binary 50 0 --gtest_filter=*mpi_mixed_speed_test*
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/twitter_ff_query10_binary 100 0 --gtest_filter=*mpi_mixed_speed_test*
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/twitter_ff_query10_binary 1000 0 --gtest_filter=*mpi_mixed_speed_test*
