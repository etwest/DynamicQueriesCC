#!/bin/bash

declare base_dir="$(dirname $(dirname $(realpath $0)))"

cd ${base_dir}/build
set -e
make -j
set +e

mkdir -p ./../results

# mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/erdos_0001_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/erdos_001_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/erdos_01_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/erdos_10_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/erdos_20_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/erdos_30_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/erdos_40_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/erdos_50_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/erdos_60_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/erdos_70_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/erdos_80_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/erdos_90_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
# mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/erdos_100_stream_binary 0 0 --gtest_filter=*mpi_update_speed_test*
