#!/bin/bash

declare base_dir="$(dirname $(dirname $(realpath $0)))"

cd ${base_dir}/build
set -e
make -j
set +e

mkdir -p ./../results

mpirun -np 23 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_13_stream_binary --gtest_filter=*mpi_query_speed_test*
mpirun -np 26 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_15_stream_binary --gtest_filter=*mpi_query_speed_test*
mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_16_stream_binary --gtest_filter=*mpi_query_speed_test*
mpirun -np 30 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_17_stream_binary --gtest_filter=*mpi_query_speed_test*
mpirun -np 31 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_18_stream_binary --gtest_filter=*mpi_query_speed_test*

mpirun -np 19 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/dnc_stream_binary --gtest_filter=*mpi_query_speed_test*
mpirun -np 26 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/tech_stream_binary --gtest_filter=*mpi_query_speed_test*
mpirun -np 29 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/enron_stream_binary --gtest_filter=*mpi_query_speed_test*
