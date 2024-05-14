# How to Run
Setup:
* ```$ mkdir build```
* ```$ cd build```
* ```$ mkdir binary_streams```
* ```$ ln -s /mnt/ssd2/fast_query_project/binary_streams/* binary_streams```
* ```$ cmake .. -DCMAKE_BUILD_TYPE=Release``` or release type of choice (Release, RelWithDebInfo, Debug)
* ```$ make$```

Run With Scripts:
* To run performance tests for update speed, query speed, or correctness tests, uncomment the script file to run on the input stream of your choice, and then run one of the following scripts:
* You may have to make the scripts executable with ```$ chmod +x [script_file]```
* ```$ scripts/mpi_update_test```
* ```$ scripts/mpi_query_test```
* ```$ scripts/mpi_correct_test```
* ```$ scripts/mpi_space_test```
* * In the space test script you can change the numerical value TIME in ```./../scripts/mem_record.sh mpi_dynamicCC_tests [TIME] ./../results/mpi_space_results/$2_mem.txt``` to determine how many seconds are inbetween each memory measurement

Tweaking Hyperparameters:
* Update batch size: in ```test/mpi_graph_tiers_test.cpp``` edit the ```DEFAULT_BATCH_SIZE``` variable.
* Sketch buffer size: in ```include/skiplist.h``` edit the ```skiplist_buffer_cap``` variable.
* Skiplist height: in ```test/mpi_graph_tiers_test.cpp``` in the specific test you want to run edit the ```height_factor``` and\or ```sketchless_height_factor``` variables. Note that the first variable is for the skiplists in the Euler tour trees for each tier, and the second variable is only for the single query Euler tour tree on the input node (not containing sketches).

Run OMP Version Manually:
* ```$ ./dynamicCC_tests [binary_stream_file] --gtest_filter=*[filter]*```
* Possible filters: omp_speed, omp_correct, query_speed, etc.
* Possible streams: kron_13_stream_binary, kron_15_stream_binary, etc.

Run MPI Version Manually:
* ```$ mpirun -np [num_processes] ./mpi_dynamicCC_tests [binary_stream_file] --gtest_filter=*[filter]*```
* num_processes: you can try to guess the number of processes and run it, the program will tell you the correct number for your input
* Possible filters: mpi_speed, mpi_correct, mpi_queries, etc.
* Possible streams: kron_13_stream_binary, kron_15_stream_binary, etc.

# Streaming Connected Components with Dynamic Queries
Using our previous [implementation](https://github.com/GraphStreamingProject/GraphStreamingCC) we can efficiently process a stream of edge insertions and deletions and then return the connected components of the graph. In this repo we will attempt to build an implementation that can efficiently answer queries interspersed with the stream.

# Based Upon
* [Dynamic graph connectivity in poly-logarithmic worst case time](https://dl.acm.org/doi/10.5555/2627817.2627898) Kapron, King, Mountjoy.
* Our paper here, once its published

# Data-structures

### Euler Tour Tree
Euler Tour Trees are a spanning tree representation that can be efficiently joined togther or cut apart. Additionally, they can efficient maintain sub-tree aggregate information. The key to this algorithm is that this sub-tree aggregate information can include the sum over the sketches for all nodes in the sub-tree. We can then use these sketches to sample edges in the cut of this sub-tree and the rest of the graph. This allows us to efficiently use sketches to join ETTs and to potentially repair deletions that shouldn't change the connected components.

Additionally, we increase the usefulness of the euler tour trees by having them track the highest weight edge added at a given tier for each node. This is done so that when inserting at a lower tier would cause a cycle we can delete this tracked edge. The Kapron, King, Mountjoy paper uses link-cut-trees for this purpose but we have found that it is unnecessary to do so.

### Linear Sketching
In the context of graph streaming, Linear Sketching data-structures represent a collection a edges incident to a node (or cluster of nodes) in sub-linear space. This sub-linear representation can be queried and will return an edge incident to the node (or cluster) at random.

We use our linear sketch `CubeSketch` that we describe and implement in `paper reference here` and in our previous implementation. The paper by Kapron, King, Mountjoy utilizes a different version of linear sketching. The performance of our sketching technique is a log faster in the average case.
