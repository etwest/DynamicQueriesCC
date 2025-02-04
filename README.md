# Streaming Connected Components with Dynamic Queries
This repository solves fully dynamic graph connectivity in the semi-streaming setting. It achieves fast update throughputs (millions of updates per second) and compact data-structures (orders of magnitude smaller than other dynamic connectivity systems), while also answering connectivity queries (of the form is `u` connected to `v`) in less than a 0.00001 seconds.

Cite our paper here, once published.

## Building and Running

### Requirements
- Unix OS (not Mac, tested on Ubuntu)
- cmake>=3.15
- openmpi
- openmp

### Building
1. Clone this repository
2. Create a build sub directory
3. From build run `cmake ..`
4. Run `make`
```
git clone https://github.com/etwest/DynamicQueriesCC.git
mkdir build
cd build
cmake ..
make -j
```

### Run unit tests
Run our unit tests with the following command:`./dynamicCC_tests`

## Experiments

### Acquire Data
Place instructions for downloading datasets here.

## How to Run
Setup:
- Complete build steps above
- Create a link to datasets called `binary_streams` in the build directory: `ln -s </path/to/datasets> binary_streams`

Run With Scripts:
* To run performance tests for update speed, query speed, or correctness tests, uncomment the script file to run on the input stream of your choice, and then run one of the following scripts:
* You may have to make the scripts executable with `chmod +x [script_file]`
* `scripts/mpi_update_test`
* `scripts/mpi_query_test`
* `scripts/mpi_correct_test`
* `scripts/mpi_space_test`
* In the space test script you can change the numerical value TIME in `./../scripts/mem_record.sh mpi_dynamicCC_tests [TIME] ./../results/mpi_space_results/$2_mem.txt` to determine how many seconds are inbetween each memory measurement

Tweaking Hyperparameters:
* Update batch size: in `test/mpi_graph_tiers_test.cpp` edit the `DEFAULT_BATCH_SIZE` variable.
* Sketch buffer size: in `include/skiplist.h` edit the `skiplist_buffer_cap` variable.
* Skiplist height: in `test/mpi_graph_tiers_test.cpp` in the specific test you want to run edit the `height_factor` and\or `sketchless_height_factor` variables. Note that the first variable is for the skiplists in the Euler tour trees for each tier, and the second variable is only for the single query Euler tour tree on the input node (not containing sketches).

Run OMP Version Manually:
* `./dynamicCC_tests [binary_stream_file] --gtest_filter=*[filter]*`
* Possible filters: omp_speed, omp_correct, query_speed, etc.
* Possible streams: kron_13_stream_binary, kron_15_stream_binary, etc.

Run MPI Version Manually:
* `mpirun -np [num_processes] ./mpi_dynamicCC_tests [binary_stream_file] --gtest_filter=*[filter]*`
* num_processes: you can try to guess the number of processes and run it, the program will tell you the correct number for your input
* Possible filters: mpi_speed, mpi_correct, mpi_queries, etc.
* Possible streams: kron_13_stream_binary, kron_15_stream_binary, etc.



## Data-structures

### Euler Tour Tree
Euler Tour Trees are a spanning tree representation that can be efficiently joined togther or cut apart. Additionally, they can efficient maintain sub-tree aggregate information. The key to this algorithm is that this sub-tree aggregate information can include the sum over the sketches for all nodes in the sub-tree. We can then use these sketches to sample edges in the cut of this sub-tree and the rest of the graph. This allows us to efficiently use sketches to join ETTs and to potentially repair deletions that shouldn't change the connected components.

We implement our Euler Tour Trees with reduced height skip-lists (see our paper).

### Linear Sketching
In the context of graph streaming, Linear Sketching data-structures represent a collection a edges incident to a node (or cluster of nodes) in sub-linear space. This sub-linear representation can be queried and will return an edge incident to the node (or cluster) at random.

We use the linear sketch `CameoSketch` in our implementation.
