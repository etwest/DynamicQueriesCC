#include <gtest/gtest.h>
#include <chrono>
#include <signal.h>
#include "graph_tiers.h"
#include "binary_graph_stream.h"
#include "mat_graph_verifier.h"

auto start = std::chrono::high_resolution_clock::now();
auto stop = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

static void print_metrics(int signum) {
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "\nTotal time for all updates performed (ms): " << duration.count() << std::endl;
    std::cout << "\tTotal time in Sketch update (ms): " << sketch_time/1000 << std::endl;
    std::cout << "\tTotal time in Refresh function (ms): " << refresh_time/1000 << std::endl;
    std::cout << "\t\tTime in Sketch queries (ms): " << sketch_query/1000 << std::endl;
    std::cout << "\t\tTime in LCT operations (ms): " << lct_time/1000 << std::endl;
    std::cout << "\t\tTime in ETT operations (ms): " << (ett_time+ett_find_root+ett_get_agg)/1000 << std::endl;
    std::cout << "\t\t\tETT Split and Join (ms): " << ett_time/1000 << std::endl;
    std::cout << "\t\t\tETT Find Tree Root (ms): " << ett_find_root/1000 << std::endl;
    std::cout << "\t\t\tETT Get Aggregate (ms): " << ett_get_agg/1000 << std::endl;
    std::cout << "Total number of tiers grown: " << tiers_grown << std::endl;
    exit(signum);
}

TEST(GraphTiersSuite, mini_correctness_test) {
    node_id_t numnodes = 10;
    GraphTiers gt(numnodes);
    MatGraphVerifier gv(numnodes);

    // Link all of the nodes into 1 connected component
    for (node_id_t i = 0; i < numnodes-1; i++) {
        gt.update({{i, i+1}, INSERT});
        gv.edge_update(i,i+1);
        std::vector<std::set<node_id_t>> cc = gt.get_cc();
        try {
            gv.reset_cc_state();
            gv.verify_soln(cc);
        } catch (IncorrectCCException& e) {
            std::cout << "Incorrect cc found after linking nodes " << i << " and " << i+1 << std::endl;
            std::cout << "GOT: " << cc.size() << " components, EXPECTED: " << numnodes-i-1 << " components" << std::endl;
            FAIL();
        }
    }
    // One by one cut all of the nodes into singletons
    for (node_id_t i = 0; i < numnodes-1; i++) {
        gt.update({{i, i+1}, DELETE});
        gv.edge_update(i,i+1);
        std::vector<std::set<node_id_t>> cc = gt.get_cc();
        try {
            gv.reset_cc_state();
            gv.verify_soln(cc);
        } catch (IncorrectCCException& e) {
            std::cout << "Incorrect cc found after cutting nodes " << i << " and " << i+1 << std::endl;
            std::cout << "GOT: " << cc.size() << " components, EXPECTED: " << i+2 << " components" << std::endl;
            FAIL();
        }
    }
}

TEST(GraphTiersSuite, full_correctness_test) {
    try {

        BinaryGraphStream stream("kron_13_stream_binary", 100000);
        GraphTiers gt(stream.nodes());
        int edgecount = stream.edges();
        MatGraphVerifier gv(stream.nodes());
        start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < edgecount; i++) {
            GraphUpdate update = stream.get_edge();
            gt.update(update);
            gv.edge_update(update.edge.src, update.edge.dst);
            unlikely_if(i%10000 == 0 || i == edgecount-1) {
                std::vector<std::set<node_id_t>> cc = gt.get_cc();
                try {
                    gv.reset_cc_state();
                    gv.verify_soln(cc);
                    std::cout << "Update " << i << ", CCs correct." << std::endl;
                } catch (IncorrectCCException& e) {
                    std::cout << "Incorrect connected components found at update "  << i << std::endl;
		            std::cout << "GOT: " << cc.size() << std::endl;
                    FAIL();
                }
            }
        }

        print_metrics(0);

    } catch (BadStreamException& e) {
        std::cout << "ERROR: Stream binary file not found." << std::endl;
    }
}

TEST(GraphTiersSuite, update_speed_test) {
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
                std::cout << "Update " << i << ", Time:  " << duration.count() << std::endl;
            }
        }

        print_metrics(0);

    } catch (BadStreamException& e) {
        std::cout << "ERROR: Stream binary file not found." << std::endl;
    }
}

TEST(GraphTiersSuite, query_speed_test) {
    try {
        BinaryGraphStream stream("kron_13_stream_binary", 100000);
        int nodecount = stream.nodes();
        GraphTiers gt(nodecount);
        int edgecount = 150000;

        std::cout << "Building up graph..." <<  std::endl;
        for (int i = 0; i < edgecount; i++) {
            GraphUpdate update = stream.get_edge();
            gt.update(update);
        }

        int querycount = 1000000;
        int seed = time(NULL);
        srand(seed);
        std::cout << "Performing queries..." << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < querycount; i++) {
            gt.is_connected(rand()%nodecount, rand()%nodecount);
        }
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        std::cout << querycount << " Connectivity Queries, Time:  " << duration.count() << std::endl;
        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < querycount/100; i++) {
            gt.get_cc();
        }
        stop = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        std::cout << querycount/100 << " Connected Components Queries, Time:  " << duration.count() << std::endl;


    } catch (BadStreamException& e) {
        std::cout << "ERROR: Stream binary file not found." << std::endl;
    }
}
