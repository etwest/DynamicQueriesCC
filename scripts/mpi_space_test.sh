#!/bin/bash

declare base_dir="$(dirname $(dirname $(realpath $0)))"

cd ${base_dir}/build
set -e
make -j
set +e

mkdir -p ./../results
mkdir -p ./../results/mpi_space_results

run_mem_test() {
	mpirun -np $1 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/$2 0 0 --gtest_filter=*mpi_update_speed_test* &
	./../scripts/mem_record.sh mpi_dynamicCC_tests 2 ./../results/mpi_space_results/$2_mem.txt
	wait
}

run_mem_test_no_reduced_height() {
	mpirun -np $1 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/$2 0 1 --gtest_filter=*mpi_update_speed_test* &
	./../scripts/mem_record.sh mpi_dynamicCC_tests 2 ./../results/mpi_space_results/$2_no_reduced_height_mem.txt
	wait
}

run_mem_test "23" "kron_13_stream_binary"
run_mem_test "26" "kron_15_stream_binary"
run_mem_test "28" "kron_16_stream_binary"
run_mem_test "30" "kron_17_stream_binary"
run_mem_test "31" "kron_18_stream_binary"
run_mem_test "19" "dnc_stream_binary"
run_mem_test "26" "tech_stream_binary"
run_mem_test "29" "enron_stream_binary"
run_mem_test "19" "dnc_streamified_binary"
run_mem_test "26" "tech_streamified_binary"
run_mem_test "29" "enron_streamified_binary"

run_mem_test_no_reduced_height "23" "kron_13_stream_binary"
run_mem_test_no_reduced_height "26" "kron_15_stream_binary"
run_mem_test_no_reduced_height "28" "kron_16_stream_binary"
run_mem_test_no_reduced_height "30" "kron_17_stream_binary"
run_mem_test_no_reduced_height "31" "kron_18_stream_binary"
run_mem_test_no_reduced_height "19" "dnc_stream_binary"
run_mem_test_no_reduced_height "26" "tech_stream_binary"
run_mem_test_no_reduced_height "29" "enron_stream_binary"
run_mem_test_no_reduced_height "19" "dnc_streamified_binary"
run_mem_test_no_reduced_height "26" "tech_streamified_binary"
run_mem_test_no_reduced_height "29" "enron_streamified_binary"
