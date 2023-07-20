#include <gtest/gtest.h>
#include <chrono>
#include <signal.h>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include "graph_tiers.h"
#include "binary_graph_stream.h"
#include "mat_graph_verifier.h"
#include "util.h"


TEST(GraphTiersSuite, mpi_correctness_test) {
    int world_rank_buf;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank_buf);
    uint32_t world_rank = world_rank_buf;
    int world_size_buf;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size_buf);
    uint32_t world_size = world_size_buf;

    int batch_size = 100;
    BinaryGraphStream stream(stream_file, 100000);
    uint32_t num_tiers = log2(stream.nodes())/(log2(3)-1);
    if (world_size != num_tiers+1) {
        FAIL() << "MPI world size too small for graph with " << stream.nodes() << " vertices. Correct world size is: " << num_tiers+1;
    }

    if (world_rank == 0) {
        InputNode input_node(stream.nodes(), num_tiers, batch_size);
        MatGraphVerifier gv(stream.nodes());
        int edgecount = stream.edges();
	    edgecount = 1000000;
        for (int i = 0; i < edgecount; i++) {
            // Read an update from the stream and have the input node process it
            GraphUpdate update = stream.get_edge();
            input_node.update(update);
            // Correctness testing by performing a cc query
            gv.edge_update(update.edge.src, update.edge.dst);
            unlikely_if(i%10000 == 0 || i == edgecount-1) {
                std::vector<std::set<node_id_t>> cc = input_node.cc_query();
                try {
                    gv.reset_cc_state();
                    gv.verify_soln(cc);
                    std::cout << "Update " << i << ", CCs correct." << std::endl;
                } catch (IncorrectCCException& e) {
                    std::cout << "Incorrect connected components found at update "  << i << std::endl;
                    std::cout << "GOT: " << cc.size() << std::endl;
                    StreamMessage stream_message;
                    stream_message.type = END;
                    MPI_Bcast(&stream_message, sizeof(StreamMessage), MPI_BYTE, 0, MPI_COMM_WORLD);
                    FAIL();
                }
            }
        }
        std::ofstream file;
        file.open ("mpi_kron_results.txt", std::ios_base::app);
        file << stream_file << " passed correctness test." << std::endl;
        file.close();
        // Communicate to all other nodes that the stream has ended
        input_node.end();

    } else if (world_rank < num_tiers+1) {
        TierNode tier_node(stream.nodes(), world_rank-1, num_tiers, batch_size);
        tier_node.main();
    }
}

TEST(GraphTierSuite, mpi_speed_test) {
    int world_rank_buf;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank_buf);
    uint32_t world_rank = world_rank_buf;
    int world_size_buf;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size_buf);
    uint32_t world_size = world_size_buf;

    int batch_size = 100;
    BinaryGraphStream stream(stream_file, 100000);
    uint32_t num_tiers = log2(stream.nodes())/(log2(3)-1);
    if (world_size != num_tiers+1) {
        FAIL() << "MPI world size too small for graph with " << stream.nodes() << " vertices. Correct world size is: " << num_tiers+1;
    }

    if (world_rank == 0) {
        long time = 0;
        InputNode input_node(stream.nodes(), num_tiers, batch_size);
        int edgecount = stream.edges();
	    edgecount = 1000000;
        for (int i = 0; i < 300000; i++) {
            // Read an update from the stream and have the input node process it
            GraphUpdate update = stream.get_edge();
            input_node.update(update);
            unlikely_if(i%100000 == 0 || i == edgecount-1) {
                std::cout << "BUILDING UP GRAPH..." << std::endl;
            }
        }
        START(timer);
        for (int i = 0; i < edgecount; i++) {
            // Read an update from the stream and have the input node process it
            GraphUpdate update = stream.get_edge();
            input_node.update(update);
            unlikely_if(i%100000 == 0 || i == edgecount-1) {
                std::cout << "FINISHED UPDATE " << i << " OUT OF " << edgecount << " IN " << stream_file << std::endl;
            }
        }
        // Communicate to all other nodes that the stream has ended
        input_node.end();
        STOP(time, timer);
        std::cout << "TOTAL TIME FOR ALL " << edgecount << " UPDATES (ms): " << time/1000 << std::endl;
        std::ofstream file;
        file.open ("mpi_kron_results.txt", std::ios_base::app);
        file << stream_file << " updates/s: " << 1000*edgecount/(time/1000) << std::endl;
        file.close();

    } else if (world_rank < num_tiers+1) {
        TierNode tier_node(stream.nodes(), world_rank-1, num_tiers, batch_size);
        tier_node.main();
    }
}
