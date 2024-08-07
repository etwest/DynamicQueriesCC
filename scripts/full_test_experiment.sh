#!/bin/bash

declare base_dir="$(dirname $(dirname $(realpath $0)))"

cd ${base_dir}/build
#set -e
#cmake -DSKETCH_BUFFER_SIZE=25 ..
#make -j
#set +e

mkdir -p ./../results
mkdir -p ./../results/mpi_speed_results

WHICH=$1

# Test run
# mpirun -np 23 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_13_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*

case $WHICH in 

	1)
# Full sweep
	mpirun -np 23 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_13_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	2)
	mpirun -np 26 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_15_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	3)
	mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_16_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	4)
	mpirun -np 30 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_17_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	5)
	# mpirun -np 31 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_18_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	6)
	mpirun -np 19 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/dnc_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	7)
	mpirun -np 26 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/tech_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	8)
	mpirun -np 29 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/enron_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	9)
	mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/twitter_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	10)
	mpirun -np 31 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/stanford_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	11)
	mpirun -np 32 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/random2N_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	12)
	mpirun -np 29 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/randomNLOGN_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	13)
	mpirun -np 25 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/randomNSQRTN_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	14)
	mpirun -np 29 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/randomDIV_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	# Fixed-forest
	15)
	mpirun -np 23 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_13_ff_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	16)
	mpirun -np 26 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_15_ff_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	17)
	mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_16_ff_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	18)
	mpirun -np 30 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_17_ff_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	19)
	# mpirun -np 31 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/kron_18_ff_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;	
	20)
	mpirun -np 19 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/dnc_ff_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	21)
	mpirun -np 26 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/tech_ff_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	22)
	mpirun -np 29 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/enron_ff_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	23)
	mpirun -np 28 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/twitter_ff_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	24)
	mpirun -np 31 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/stanford_ff_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	25)
	mpirun -np 32 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/random2N_ff_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	25)
	mpirun -np 29 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/randomNLOGN_ff_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	26)
	mpirun -np 25 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/randomNSQRTN_ff_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
	27)
	mpirun -np 29 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/randomDIV_ff_query10_binary 0 0 --gtest_filter=*mpi_mixed_speed_test*
	;;
esac


exit
	
# Tests including memory measurement
run_mem_test() {
	mpirun -np $1 --bind-to hwthread ./mpi_dynamicCC_tests binary_streams/$2 0 0 --gtest_filter=*mpi_mixed_speed_test* &
	./../scripts/mem_record.sh mpi_dynamicCC_tests 2 ./../results/mpi_space_results/batch_size_sweep/$2_$3_mem.txt
	wait
}
	
	# # Full sweep
	# run_mem_test "23" "kron_13_query10_binary"
	# run_mem_test "26" "kron_15_query10_binary"
	# run_mem_test "28" "kron_16_query10_binary"
# run_mem_test "30" "kron_17_query10_binary"
# # run_mem_test "31" "kron_18_query10_binary"

# run_mem_test "19" "dnc_query10_binary"
# run_mem_test "26" "tech_query10_binary"
# run_mem_test "29" "enron_query10_binary"

# run_mem_test "28" "twitter_query10_binary"
# run_mem_test "31" "stanford_query10_binary"
# run_mem_test "32" "random2N_query10_binary"
# run_mem_test "29" "randomNLOGN_query10_binary"
# run_mem_test "25" "randomNSQRTN_query10_binary"
# run_mem_test "29" "randomDIV_query10_binary"

# # Fixed-forest
# run_mem_test "23" "kron_13_ff_query10_binary"
# run_mem_test "26" "kron_15_ff_query10_binary"
# run_mem_test "28" "kron_16_ff_query10_binary"
# run_mem_test "30" "kron_17_ff_query10_binary"
# # run_mem_test "31" "kron_18_ff_query10_binary"

# run_mem_test "19" "dnc_ff_query10_binary"
# run_mem_test "26" "tech_ff_query10_binary"
# run_mem_test "29" "enron_ff_query10_binary"

# run_mem_test "28" "twitter_ff_query10_binary"
# run_mem_test "31" "stanford_ff_query10_binary"
# run_mem_test "32" "random2N_ff_query10_binary"
# run_mem_test "29" "randomNLOGN_ff_query10_binary"
# run_mem_test "25" "randomNSQRTN_ff_query10_binary"
# run_mem_test "29" "randomDIV_ff_query10_binary"
