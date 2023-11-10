#include <gtest/gtest.h>
#include <chrono>
#include <signal.h>
#include <omp.h>
#include <iostream>
#include <fstream>
#include "graph_tiers.h"
#include "binary_graph_stream.h"
#include "mat_graph_verifier.h"
#include "util.h"

auto start = std::chrono::high_resolution_clock::now();
auto stop = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

static void print_metrics() {
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "\nTotal time for all updates performed (ms): " << duration.count() << std::endl;
    std::cout << "\tTotal time in Sketch update (ms): " << sketch_time/1000 << std::endl;
    std::cout << "\tTotal time in Refresh function (ms): " << refresh_time/1000 << std::endl;
    std::cout << "\t\tTime in Parallel isolated checking (ms): " << parallel_isolated_check/1000 << std::endl;
    std::cout << "\t\tTime in Sketch queries (ms): " << sketch_query/1000 << std::endl;
    std::cout << "\t\tTime in LCT operations (ms): " << lct_time/1000 << std::endl;
    std::cout << "\t\tTime in ETT operations (ms): " << (ett_time+ett_find_root+ett_get_agg)/1000 << std::endl;
    std::cout << "\t\t\tETT Split and Join (ms): " << ett_time/1000 << std::endl;
    std::cout << "\t\t\tETT Find Tree Root (ms): " << ett_find_root/1000 << std::endl;
    std::cout << "\t\t\tETT Get Aggregate (ms): " << ett_get_agg/1000 << std::endl;
    std::cout << "Total number of tiers grown: " << tiers_grown << std::endl;
    std::cout << "Total number of sketches updates: " << num_sketch_updates << std::endl;
    std::cout << "Total number of batches processed: " << num_sketch_batches << std::endl;
}

TEST(GraphTiersSuite, mini_correctness_test) {
    node_id_t numnodes = 10;
    GraphTiers gt(numnodes, false);
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

TEST(GraphTiersSuite, deletion_replace_correctness_test) {
    node_id_t numnodes = 100;
    GraphTiers gt(numnodes, false);
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
    // Generate a random bridge
    node_id_t first = rand() % numnodes;
    node_id_t second = rand() % numnodes;
    while(first == second || second == first+1 || first == second+1)
        second = rand() % numnodes;

    gt.update({{first, second}, INSERT});
    gv.edge_update(first, second);

    node_id_t distance = std::max(first, second) - std::min(first, second);
    // Cut a random edge
    first = std::min(first, second) + rand() % (distance-1);

    gt.update({{first, first+1}, DELETE});
    gv.edge_update(first, first+1);

    std::vector<std::set<node_id_t>> cc = gt.get_cc();
    try {
        gv.reset_cc_state();
        gv.verify_soln(cc);
    } catch (IncorrectCCException& e) {
        std::cout << "Incorrect cc found after cutting nodes " << first << " and " << first+1 << std::endl;
        std::cout << "GOT: " << cc.size() << " components, EXPECTED: 1 components" << std::endl;
        FAIL();
    }
}

TEST(GraphTiersSuite, omp_correctness_test) {
    omp_set_dynamic(1);
    try {

        BinaryGraphStream stream(stream_file, 100000);
        GraphTiers gt(stream.nodes(), true);
        int edgecount = stream.edges();
        edgecount = 1000000;
        MatGraphVerifier gv(stream.nodes());
        start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < edgecount; i++) {
            GraphUpdate update = stream.get_edge();
            gt.update(update);
            gv.edge_update(update.edge.src, update.edge.dst);
            unlikely_if(i%100000 == 0 || i == edgecount-1) {
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
        std::ofstream file;
        file.open ("omp_kron_results.txt", std::ios_base::app);
        file << stream_file << " passed correctness test." << std::endl;
        file.close();

    } catch (BadStreamException& e) {
        std::cout << "ERROR: Stream binary file not found." << std::endl;
    }
}

TEST(GraphTiersSuite, omp_speed_test) {
    omp_set_dynamic(1);
    try {

	    long time = 0;
        BinaryGraphStream stream(stream_file, 100000);
        GraphTiers gt(stream.nodes(), true);
        int edgecount = stream.edges();
        edgecount = 1000000;
        start = std::chrono::high_resolution_clock::now();

	    START(timer);
        for (int i = 0; i < edgecount; i++) {
            GraphUpdate update = stream.get_edge();
            gt.update(update);
            unlikely_if (i % 100000 == 0) {
                auto stop = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
                std::cout << "Update " << i << ", Time:  " << duration.count() << std::endl;
            }
        }
	    STOP(time, timer);
        print_metrics();
        std::ofstream file;
        file.open ("omp_kron_results.txt", std::ios_base::app);
        file << stream_file << " time (ms): "<< time/1000 << std::endl;
        file.close();
    } catch (BadStreamException& e) {
        std::cout << "ERROR: Stream binary file not found." << std::endl;
    }
}

TEST(GraphTiersSuite, query_speed_test) {
    omp_set_dynamic(1);
    try {

        BinaryGraphStream stream(stream_file, 100000);
        int nodecount = stream.nodes();
        GraphTiers gt(nodecount, true);
        int edgecount = 17500000;

        int seed = time(NULL);
        srand(seed);
        int querycount = 1000000;
        int running_count = querycount;
        int cc_running_count = querycount/100;

        double total_querytime = 0;
        double total_ccquerytime = 0;

        auto start = std::chrono::high_resolution_clock::now();
        auto stop = std::chrono::high_resolution_clock::now();
       
        std::cout << "Building up graph and performing queries..." <<  std::endl;
        for (int i = 0; i < edgecount; i++) {
            GraphUpdate update = stream.get_edge();
            gt.update(update);

            if(running_count == 0) {
                break;
            }

            // time taken to perform connectivity query
            if (running_count > 0  && static_cast<double>(std::rand()) / RAND_MAX < .3) {
                start = std::chrono::high_resolution_clock::now();
                gt.is_connected(rand()%nodecount, rand()%nodecount);
                stop = std::chrono::high_resolution_clock::now();

                total_querytime += std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();

                if (cc_running_count > 0 && static_cast<double>(std::rand()) / RAND_MAX < .5) {
                    start = std::chrono::high_resolution_clock::now();
                    gt.get_cc();
                    stop = std::chrono::high_resolution_clock::now();

                    total_ccquerytime += std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
                    --cc_running_count;
                }
                --running_count;
            }
              
        }

        int query_trials = querycount - running_count;
        double querytime_seconds = total_querytime / 1e6;
        int cc_trials = (querycount/100) - cc_running_count;
        double cc_time_seconds = total_ccquerytime / 1e6;

        std::cout << query_trials << " Connectivity Queries, Time:  " << querytime_seconds  << "s, " << query_trials/querytime_seconds << " queries/s" << std::endl;
        std::cout << cc_trials << " Connected Components Queries, Time:  " << cc_time_seconds << "s, " << cc_trials/cc_time_seconds << " queries/s" << std::endl;

    } catch (BadStreamException& e) {
        std::cout << "ERROR: Stream binary file not found." << std::endl;
    }
}