#!/bin/bash

declare base_dir="$(dirname $(dirname $(realpath $0)))"
echo "Testing with buffer size $1"

cd ${base_dir}/build
set -e
cmake -DSKETCH_BUFFER_SIZE=$1 ..
make -j
set +e

mkdir -p ./../results
mkdir -p ./../results/mpi_space_results
mkdir -p ./../results/mpi_space_results/batch_size_sweep

run_mem_test() {
	mpirun -np $1 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/$2 0 0 --gtest_filter=*mpi_mixed_speed_test* &
	./../scripts/mem_record.sh mpi_dynamicCC_tests 2 ./../results/mpi_space_results/batch_size_sweep/$2_$3_mem.txt
	wait
}

# Test run
# run_mem_test "23" "kron_13_query10_binary" $1

# KRON-16 Batch Size Sweep
run_mem_test "28" "kron_16_query10_binary" $1
# KRON-16 fixed-forest
run_mem_test "28" "kron_16_ff_query10_binary" $1

# Twitter Batch Size Sweep
run_mem_test "28" "twitter_query10_binary" $1
# Twitter fixed-forest
run_mem_test "28" "twitter_ff_query10_binary" $1
