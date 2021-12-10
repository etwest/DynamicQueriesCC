# Graph Streaming Connected Components with Dynamic Queries
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

# Implementation TODOs
1. Euler Tour Tree implementation that supports
    1. sketch aggregates
    2. tracking new relevent edges
2. Organizing the Euler Tour Trees into different tiers
3. Logic for updating the ETT aggregate sketches on insert and delete
4. Implementations of the key functions
    1. Insert
    2. Delete
    3. Refresh
    4. Query
