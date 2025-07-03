#! /usr/bin/bash

if [[ $# -ne 1 ]]; then
  echo "Incorrect number of arguments. Require: path_to"
  echo "path_to : path to the binary stream files"
  exit 1
fi

location=$1

####### Query 10 datasets #######

# kron
./idtree_expr binary-file-stream $location/kron_13_query10_binary
mv dyn_results.txt kron_13_query10_result.txt

./idtree_expr binary-file-stream $location//kron_15_query10_binary
mv dyn_results.txt kron_15_query10_result.txt

./idtree_expr binary-file-stream $location/kron_16_query10_binary
mv dyn_results.txt kron_16_query10_result.txt

./idtree_expr binary-file-stream $location/kron_17_query10_binary
mv dyn_results.txt kron_17_query10_result.txt

#./idtree_expr binary-file-stream $location/kron_18_query10_binary
#mv dyn_results.txt kron_18_query10_result.txt

# real world
./idtree_expr binary-file-stream $location/dnc_query10_binary
mv dyn_results.txt dnc_email_query10_result.txt

./idtree_expr binary-file-stream $location/enron_query10_binary
mv dyn_results.txt enron_query10_result.txt

./idtree_expr binary-file-stream $location/stanford_query10_binary
mv dyn_results.txt standford_query10_result.txt

./idtree_expr binary-file-stream $location/tech_query10_binary
mv dyn_results.txt tech_query10_result.txt

./idtree_expr binary-file-stream $location/twitter_query10_binary
mv dyn_results.txt twitter_query10_result.txt

# dtree random
./idtree_expr binary-file-stream $location/random2N_query10_binary
mv dyn_results.txt random2N_query10_result.txt

./idtree_expr binary-file-stream $location/randomNLOGN_query10_binary
mv dyn_results.txt randomNLOGN_query10_result.txt

./idtree_expr binary-file-stream $location/randomNSQRTN_query10_binary
mv dyn_results.txt randomNSQRTN_query10_result.txt

./idtree_expr binary-file-stream $location/randomDIV_query10_binary
mv dyn_results.txt randomDIV_query10_result.txt

####### fixed forest query10 #######

# kron
./idtree_expr binary-file-stream $location/kron_13_ff_query10_binary
mv dyn_results.txt kron_13_ff_query10_result.txt

./idtree_expr binary-file-stream $location/kron_15_ff_query10_binary
mv dyn_results.txt kron_15_ff_query10_result.txt

./idtree_expr binary-file-stream $location/kron_16_ff_query10_binary
mv dyn_results.txt kron_16_ff_query10_result.txt

./idtree_expr binary-file-stream $location/kron_17_ff_query10_binary
mv dyn_results.txt kron_17_ff_query10_result.txt

#./idtree_expr binary-file-stream $location/kron_18_ff_query10_binary
#mv dyn_results.txt kron_18_ff_query10_result.txt

# real world
./idtree_expr binary-file-stream $location/dnc_ff_query10_binary
mv dyn_results.txt dnc_email_ff_query10_result.txt

./idtree_expr binary-file-stream $location/enron_ff_query10_binary
mv dyn_results.txt enron_ff_query10_result.txt

./idtree_expr binary-file-stream $location/stanford_ff_query10_binary
mv dyn_results.txt standford_ff_query10_result.txt

./idtree_expr binary-file-stream $location/tech_ff_query10_binary
mv dyn_results.txt tech_ff_query10_result.txt

./idtree_expr binary-file-stream $location/twitter_ff_query10_binary
mv dyn_results.txt twitter_ff_query10_result.txt

# dtree random
./idtree_expr binary-file-stream $location/random2N_ff_query10_binary
mv dyn_results.txt random2N_ff_query10_result.txt

./idtree_expr binary-file-stream $location/randomNLOGN_ff_query10_binary
mv dyn_results.txt randomNLOGN_ff_query10_result.txt

./idtree_expr binary-file-stream $location/randomNSQRTN_ff_query10_binary
mv dyn_results.txt randomNSQRTN_ff_query10_result.txt

./idtree_expr binary-file-stream $location/randomDIV_ff_query10_binary
mv dyn_results.txt randomDIV_ff_query10_result.txt