#include <gtest/gtest.h>
#include <chrono>
#include "graph_tiers.h"
#include "binary_graph_stream.h"

TEST(GraphTiersSuite, read_from_binary) {
    BinaryGraphStream stream("kron_13_stream_binary", 100000);
}

// TEST(GraphTiersSuite, cc_correctness_test) {
//     BinaryGraphStream stream("kron_13_stream_binary", 100000);
//     GraphTiers gt(stream.nodes());
//     int edgecount = stream.edges();

//     for (int i = 0; i < edgecount; i++) {
//         GraphUpdate update = stream.get_edge();
//         gt.update(update);
//         unlikely_if (i % 10000 == 0) {
//             std::cout << "Update " << i << ", Components:  " << "\n";
//             std::vector<std::vector<node_id_t>> cc = gt.get_cc();
//             for (auto component : cc) {
//                 for (auto node : component) {
//                     std::cout << node << " ";
//                 }
//                 std::cout << "\n";
//             }
//         }
//     }
// }

TEST(GraphTiersSuite, cc_speed_test) {
    BinaryGraphStream stream("kron_13_stream_binary", 100000);
    GraphTiers gt(stream.nodes());
    int edgecount = stream.edges();
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < edgecount; i++) {
        GraphUpdate update = stream.get_edge();
        gt.update(update);
        unlikely_if (i % 10000 == 0) {
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            std::cout << "Update " << i << ", Time:  " << duration.count()/1000000 << "\n";
        }
    }

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "Total time for " << stream.edges() << " updates: " << duration.count() << std::endl;
}
