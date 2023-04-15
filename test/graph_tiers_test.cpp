#include <gtest/gtest.h>
#include <chrono>
#include <signal.h>
#include "graph_tiers.h"
#include "binary_graph_stream.h"
#include "mat_graph_verifier.h"

auto start = std::chrono::high_resolution_clock::now();
auto stop = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

TEST(GraphTiersSuite, cc_correctness_test) {
    try {

        BinaryGraphStream stream("kron_13_stream_binary", 100000);
        GraphTiers gt(stream.nodes());
        int edgecount = stream.edges();

        MatGraphVerifier gv(stream.nodes());

        for (int i = 0; i < edgecount; i++) {
            GraphUpdate update = stream.get_edge();
            gt.update(update);
            gv.edge_update(update.edge.src, update.edge.dst);
            unlikely_if(i%100000 == 0 || i == edgecount-1) {
                gv.reset_cc_state();
                std::vector<std::set<node_id_t>> cc = gt.get_cc();
                try {
                    gv.verify_soln(cc);
                    std::cout << "Update " << i << ", CC correct.\n";
                } catch (IncorrectCCException& e) {
                    std::cout << "Incorrect connected components!" << "\n";
                    std::cout << "Update " << i << " " << update.edge.src << " " << update.edge.dst << " "
                    << (update.type ? "DELETE" : "INSERT") << "\n";
                    // for (auto component : cc) {
                    //     std::cout << "[";
                    //     for (auto node : component) {
                    //         std::cout << node << " ";
                    //     }
                    //     std::cout << "] ";
                    // }
                    // std::cout << "\nGround truth components:\n";
                    // for (auto component : gv.kruskal_ref) {
                    //     std::cout << "[";
                    //     for (auto node : component) {
                    //         std::cout << node << " ";
                    //     }
                    //     std::cout << "] ";
                    // }
                    FAIL();
                }
            }
        }

    } catch (BadStreamException& e) {
        std::cout << "ERROR: Stream binary file not found." << std::endl;
    }
}

static void print_metrics(int signum) {
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "\nTotal time for all updates performed (ms): " << duration.count() << std::endl;
    std::cout << "\tTotal time in Sketch update (ms): " << sketch_time/1000 << std::endl;
    std::cout << "\tTotal time in Refresh function (ms): " << refresh_time/1000 << std::endl;
    std::cout << "\t\tTime in Sketch queries (ms): " << sketch_query/1000 << std::endl;
    std::cout << "\t\tTime in LCT operations (ms): " << lct_time/1000 << std::endl;
    std::cout << "\t\tTime in ETT operations (ms): " << (ett_time+ett_find_root)/1000 << std::endl;
    std::cout << "\t\t\tETT Split and Join (ms):" << ett_time/1000 << std::endl;
    std::cout << "\t\t\tETT Find Tree Root (ms):" << ett_find_root/1000 << std::endl;
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
            unlikely_if (i % 100000 == 0) {
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
