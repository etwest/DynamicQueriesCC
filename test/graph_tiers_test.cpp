#include <gtest/gtest.h>
#include <chrono>
#include <signal.h>
#include "graph_tiers.h"
#include "binary_graph_stream.h"

auto start = std::chrono::high_resolution_clock::now();
auto stop = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

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

static void print_metrics(int signum) {
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "\nTotal time for all updates performed (ms): " << duration.count() << std::endl;
    std::cout << "\tTotal time in Sketch update (ms): " << sketch_time/1000 << std::endl;
    std::cout << "\tTotal time in Refresh function (ms): " << refresh_time/1000 << std::endl;
    std::cout << "\t\tTime in Sketch queries (ms): " << sketch_query/1000 << std::endl;
    std::cout << "\t\tTime in LCT operations (ms): " << lct_time/1000 << std::endl;
    std::cout << "\t\tTime in ETT operations (ms): " << ett_time/1000 << std::endl;
    std::cout << "Total number of tiers grown: " << tiers_grown << std::endl;
    exit(signum);
}

TEST(GraphTiersSuite, cc_speed_test) {
    try {

        signal(SIGINT, print_metrics);
        BinaryGraphStream stream("kron_13_stream_binary", 100000);
        GraphTiers gt(stream.nodes());
        int edgecount = stream.edges();
        start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < edgecount; i++) {
            GraphUpdate update = stream.get_edge();
            gt.update(update);
            unlikely_if (i % 10000 == 0) {
                auto stop = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
                std::cout << "Update " << i << ", Time:  " << duration.count() << "\n";
            }
        }

        print_metrics(0);

    } catch (BadStreamException& e) {
        std::cout << "ERROR: Stream binary file not found." << std::endl;
    }
}
