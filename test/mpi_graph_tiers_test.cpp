#include <gtest/gtest.h>
#include <chrono>
#include <signal.h>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <omp.h>
#include "mpi_nodes.h"
#include "binary_graph_stream.h"
#include "mat_graph_verifier.h"
#include "util.h"


TEST(GraphTiersSuite, mpi_mini_correctness_test) {
    int world_rank_buf;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank_buf);
    uint32_t world_rank = world_rank_buf;
    int world_size_buf;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size_buf);
    uint32_t world_size = world_size_buf;

    uint32_t num_nodes = 100;
    uint32_t num_tiers = log2(num_nodes)/(log2(3)-1);
    if (world_size != num_tiers+1)
        FAIL() << "MPI world size too small for graph with " << num_nodes << " vertices. Correct world size is: " << num_tiers+1;
    // Parameters
    int update_batch_size = 100;
    // skiplist_buffer_cap = 10;
    height_factor = 4./num_tiers;
    vec_t sketch_len = ((vec_t)num_nodes*num_nodes);
	vec_t sketch_err = 4;

	// Configure the sketches globally
	Sketch::configure(sketch_len, sketch_err);

    if (world_rank == 0) {
        InputNode input_node(num_nodes, num_tiers, update_batch_size);
        MatGraphVerifier gv(num_nodes);
        // Link all of the nodes into 1 connected component
        for (node_id_t i = 0; i < num_nodes-1; i++) {
            input_node.update({{i, i+1}, INSERT});
            gv.edge_update(i,i+1);
            std::vector<std::set<node_id_t>> cc = input_node.cc_query();
            try {
                gv.reset_cc_state();
                gv.verify_soln(cc);
            } catch (IncorrectCCException& e) {
                std::cout << "Incorrect cc found after linking nodes " << i << " and " << i+1 << std::endl;
                std::cout << "GOT: " << cc.size() << " components, EXPECTED: " << num_nodes-i-1 << " components" << std::endl;
                FAIL();
            }
        }
        // One by one cut all of the nodes into singletons
        for (node_id_t i = 0; i < num_nodes-1; i++) {
            input_node.update({{i, i+1}, DELETE});
            gv.edge_update(i,i+1);
            std::vector<std::set<node_id_t>> cc = input_node.cc_query();
            try {
                gv.reset_cc_state();
                gv.verify_soln(cc);
            } catch (IncorrectCCException& e) {
                std::cout << "Incorrect cc found after cutting nodes " << i << " and " << i+1 << std::endl;
                std::cout << "GOT: " << cc.size() << " components, EXPECTED: " << i+2 << " components" << std::endl;
                FAIL();
            }
        }
        // Communicate to all other nodes that the stream has ended
        input_node.end();
    } else if (world_rank < num_tiers+1) {
        TierNode tier_node(num_nodes, world_rank-1, num_tiers, update_batch_size);
        tier_node.main();
    }
}

TEST(GraphTiersSuite, mpi_correctness_test) {
    int world_rank_buf;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank_buf);
    uint32_t world_rank = world_rank_buf;
    int world_size_buf;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size_buf);
    uint32_t world_size = world_size_buf;

    BinaryGraphStream stream(stream_file, 100000);
    uint32_t num_nodes = stream.nodes();
    uint32_t num_tiers = log2(num_nodes)/(log2(3)-1);

    // Parameters
    int update_batch_size = 100;
    // skiplist_buffer_cap = 10;
    height_factor = 4./num_tiers;
    vec_t sketch_len = ((vec_t)num_nodes*num_nodes);
	vec_t sketch_err = 4;

	// Configure the sketches globally
	Sketch::configure(sketch_len, sketch_err);

    if (world_size != num_tiers+1) {
        FAIL() << "MPI world size too small for graph with " << num_nodes << " vertices. Correct world size is: " << num_tiers+1;
    }

    if (world_rank == 0) {
        InputNode input_node(num_nodes, num_tiers, update_batch_size);
        MatGraphVerifier gv(num_nodes);
        int edgecount = stream.edges();
	    int count = 20000000;
        edgecount = std::min(edgecount, count);
        for (int i = 0; i < edgecount; i++) {
            // Read an update from the stream and have the input node process it
            GraphUpdate update = stream.get_edge();
            input_node.update(update);
            // Correctness testing by performing a cc query
            gv.edge_update(update.edge.src, update.edge.dst);
            unlikely_if(i%1000 == 0 || i == edgecount-1) {
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
        TierNode tier_node(num_nodes, world_rank-1, num_tiers, update_batch_size);
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

    BinaryGraphStream stream(stream_file, 100000);
    uint32_t num_nodes = stream.nodes();
    uint32_t num_tiers = log2(num_nodes)/(log2(3)-1);

    // Parameters
    int update_batch_size = 100;
    // skiplist_buffer_cap = 10;
    height_factor = 4./num_tiers;
    vec_t sketch_len = ((vec_t)num_nodes*num_nodes);
	vec_t sketch_err = 4;

	// Configure the sketches globally
	Sketch::configure(sketch_len, sketch_err);

    if (world_size != num_tiers+1) {
        FAIL() << "MPI world size too small for graph with " << num_nodes << " vertices. Correct world size is: " << num_tiers+1;
    }

    if (world_rank == 0) {
        long time = 0;
        InputNode input_node(num_nodes, num_tiers, update_batch_size);
        long edgecount = stream.edges();
        long count = 18000000;
        edgecount = std::min(edgecount, count);
        START(timer);
        for (long i = 0; i < edgecount; i++) {
            // Read an update from the stream and have the input node process it
            GraphUpdate update = stream.get_edge();
            input_node.update(update);
            unlikely_if(i%1000000 == 0 || i == edgecount-1) {
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
        TierNode tier_node(num_nodes, world_rank-1, num_tiers, update_batch_size);
        tier_node.main();
    }
}
