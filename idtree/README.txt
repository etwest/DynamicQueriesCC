Step 1: Compile

For our method:
g++ DynamicCC.cpp -o DynamicCC -O3 =c++1y

Step 2: Format the dataset:

For static graph:
./DynamicCC txt-to-bin ../test_graph/

For temporal graph:
./DynamicCC txt-to-stream ../test_stream/

Step 3: Run update
For static graph: (random delete 10 edges and then add them back. In DynamicCC, 0 means ID-tree, 1 means DND-trees )

./DynamicCC sample ../test_graph/ 10 0
./DynamicCC sample ../test_graph/ 10 1

For temporal graph: (window size = 5 means 5% of the time span of the dataset, if window size = -2 means that all edges are inserted first, and then all edges are deleted.)

./DynamicCC stream ../test_stream/ 5 0
./DynamicCC stream ../test_stream/ 5 1

Step 4: Run query
For static graph: (random generate 100 queries)

./DynamicCC query-random ../test_graph/ 100 0
./DynamicCC query-random ../test_graph/ 100 1


For temporal graph: (window size = 5 means 5% of the time span of the dataset.  In the last window, we random generate 100 queries.)

./DynamicCC stream ../test_stream/ 5 100 0
./DynamicCC stream ../test_stream/ 5 100 1
