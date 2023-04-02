#include <gtest/gtest.h>
#include "graph_tiers.h"
#include "binary_graph_stream.h"

TEST(GraphTiersSuite, read_from_binary) {
    BinaryGraphStream stream("kron_13_stream_binary", 100000);
    GraphTiers gt(stream.nodes());
    int edgecount = stream.edges();

    for (int i = 0; i < edgecount; i++) {
        GraphUpdate update = stream.get_edge();
        std::cout << update.type << " " << update.edge.src << " " << update.edge.dst << std::endl;
        gt.update(update);
    }
}