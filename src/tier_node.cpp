#include "../include/mpi_nodes.h"

long sketch_update_time = 0;
long greedy_refresh_time = 0;
long normal_refresh_time = 0;

long initial_bcast_time = 0;
long greedy_gather_time = 0;
long greedy_bcast_time = 0;

TierNode::TierNode(node_id_t num_nodes, uint32_t tier_num, uint32_t num_tiers, int batch_size) :
    tier_num(tier_num), num_tiers(num_tiers) {
    // Algorithm parameters
	vec_t sketch_len = ((vec_t)num_nodes) * num_nodes;
	vec_t sketch_err = 10;
	int seed = time(NULL);

	// Configure the sketches globally
	Sketch::configure(sketch_len, sketch_err);

	// Initialize all the ETT node
    srand(seed);
    ett_nodes.reserve(num_nodes);
    for (node_id_t i = 0; i < num_nodes; ++i) {
        ett_nodes.emplace_back(seed, i, tier_num);
    }

    update_buffer.reserve(batch_size + 1);
}

void TierNode::main() {
    while (true) {
        // Receive a batch of updates and check if it is the end of stream
        START(initial_bcast_timer);
        bcast(&update_buffer[0], sizeof(StreamMessage)*update_buffer.capacity(), 0);
        STOP(initial_bcast_time, initial_bcast_timer);
        if (update_buffer[0].type == END) {
            std::cout << "============= TIER " << tier_num << " NODE =============" << std::endl;
            std::cout << "Time in initial bcast (ms): " << initial_bcast_time/1000 << std::endl;
            std::cout << "Sketch update time (ms): " << sketch_update_time/1000 << std::endl;
            std::cout << "Greedy refresh time (ms): " << greedy_refresh_time/1000 << std::endl;
            std::cout << "\tGreedy gather time (ms): " << greedy_gather_time/1000 << std::endl;
            std::cout << "\tGreedy bcast time (ms): " << greedy_bcast_time/1000 << std::endl;
            std::cout << "Normal refresh time (ms): " << normal_refresh_time/1000 << std::endl;
            return;
        }
        // Process all the updates in the batch
        for (uint32_t i = 1; i < update_buffer[0].update.edge.src; i++) {
            // Perform the sketch updating
            START(sketch_update_timer);
            GraphUpdate update = update_buffer[i].update;
            edge_id_t edge = VERTICES_TO_EDGE(update.edge.src, update.edge.dst);
            unlikely_if (update.type == DELETE && ett_nodes[update.edge.src].has_edge_to(&ett_nodes[update.edge.dst])) {
                ett_nodes[update.edge.src].cut(ett_nodes[update.edge.dst]);
            }
            SkipListNode* root1 = ett_nodes[update.edge.src].update_sketch((vec_t)edge);
            SkipListNode* root2 = ett_nodes[update.edge.dst].update_sketch((vec_t)edge);
            STOP(sketch_update_time, sketch_update_timer);
            START(greedy_refresh_timer);
            // Try the greedy parallel refresh
            GreedyRefreshMessage refresh_message;
            refresh_message.size1 = root1->size;
            root1->process_updates();
            refresh_message.query_result1 = root1->sketch_agg->query().second;
            refresh_message.size2 = root2->size;
            root2->process_updates();
            refresh_message.query_result2 = root2->sketch_agg->query().second;
            barrier();
            START(greedy_gather_timer);
            gather(&refresh_message, sizeof(GreedyRefreshMessage), nullptr, 0, 0);
            STOP(greedy_gather_time, greedy_gather_timer);
            UpdateMessage isolation_message;
            START(greedy_bcast_timer);
            bcast(&isolation_message, sizeof(UpdateMessage), 0);
            STOP(greedy_bcast_time, greedy_bcast_timer);
            STOP(greedy_refresh_time, greedy_refresh_timer);
            if (isolation_message.type == NOT_ISOLATED)
                continue;
            // Start the refreshing sequence
            START(normal_refresh_timer);
            for (uint32_t tier = 0; tier < num_tiers; tier++) {
                int rank = tier + 1;
                // If this node's tier is the current tier process the refresh message from previous tier or input node
                if (tier == tier_num) {
                    RefreshMessage refresh_message;
                    MPI_Recv(&refresh_message, sizeof(RefreshMessage), MPI_BYTE, tier_num, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    refresh_tier(refresh_message);
                    // Send a refresh message to the next tier
                    if (tier < num_tiers-1) {
                        RefreshEndpoint e1, e2;
                        e1.v = refresh_message.endpoints.first.v;
                        e2.v = refresh_message.endpoints.second.v;
                        for (RefreshEndpoint* e : {&e1, &e2}) {
                            e->prev_tier_size = ett_nodes[e->v].get_size();
                            SkipListNode* root = ett_nodes[e->v].get_root();
                            root->process_updates();
                            Sketch* ett_agg = root->sketch_agg;
                            std::pair<vec_t, SampleSketchRet> query_result = ett_agg->query();
                            e->sketch_query_result_type = query_result.second;
                            e->sketch_query_result = query_result.first;
                        }
                        RefreshMessage next_refresh_message;
                        next_refresh_message.endpoints = {e1, e2};
                        MPI_Send(&next_refresh_message, sizeof(RefreshMessage), MPI_BYTE, rank+1, 0, MPI_COMM_WORLD);
                    }
                    continue;
                }
                // For every other tier just receive and perform update messages
                for (int endpoint : {0,1}) {
                    std::ignore = endpoint;
                    // Receive a broadcast to see if the endpoint at the current tier is isolated or not
                    UpdateMessage update_message;
                    bcast(&update_message, sizeof(UpdateMessage), rank);
                    if (update_message.type == NOT_ISOLATED) continue;
                    // Get the two broadcasts and perform ett updates
                    for (int broadcast : {0,1}) {
                        std::ignore = broadcast;
                        UpdateMessage update_message;
                        bcast(&update_message, sizeof(UpdateMessage), rank);
                        ett_update_tier(update_message);
                        if (update_message.type == LINK) break;
                    }
                }
            }
            STOP(normal_refresh_time, normal_refresh_timer);
        }
        update_buffer.clear();
    }
}

void TierNode::update_tier(GraphUpdate update) {
    edge_id_t edge = VERTICES_TO_EDGE(update.edge.src, update.edge.dst);
    ett_nodes[update.edge.src].update_sketch((vec_t)edge);
    ett_nodes[update.edge.dst].update_sketch((vec_t)edge);
    unlikely_if (update.type == DELETE && ett_nodes[update.edge.src].has_edge_to(&ett_nodes[update.edge.dst])) {
        ett_nodes[update.edge.src].cut(ett_nodes[update.edge.dst]);
    }
}

void TierNode::ett_update_tier(UpdateMessage message) {
    if (message.type == LINK && tier_num > message.start_tier) {
        ett_nodes[message.endpoint1].link(ett_nodes[message.endpoint2]);
    } else if (message.type == CUT && tier_num > message.start_tier) {
        ett_nodes[message.endpoint1].cut(ett_nodes[message.endpoint2]);
    }
}

void TierNode::refresh_tier(RefreshMessage message) {
    for (RefreshEndpoint endpoint: {message.endpoints.first, message.endpoints.second}) {
        // Check if the tree containing this endpoint is isolated
        uint32_t prev_tier_size = endpoint.prev_tier_size;
        uint32_t this_tier_size = ett_nodes[endpoint.v].get_size();
        
        node_id_t a = (node_id_t)endpoint.sketch_query_result;
        node_id_t b = (node_id_t)(endpoint.sketch_query_result>>32);
        
        // Tell all other nodes an isolation was found
        UpdateMessage update_message;
        update_message.type = (TreeOperationType)(!(prev_tier_size != this_tier_size || endpoint.sketch_query_result_type != GOOD));
        update_message.endpoint1 = a;
        update_message.endpoint2 = b;
        bcast(&update_message, sizeof(UpdateMessage), tier_num+1);
				
        if (update_message.type == NOT_ISOLATED)
            continue;
				
        // Query LCT node to check if this new edge forms a cycle
        LctResponseMessage lct_response;
        MPI_Recv(&lct_response, sizeof(LctResponseMessage), MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // If there is a cycle formed, tell all necessary nodes to delete that edge
        if (lct_response.connected) {
            node_id_t c = (node_id_t)lct_response.cycle_edge;
            node_id_t d = (node_id_t)(lct_response.cycle_edge>>32);

            UpdateMessage cut_message;
            cut_message.type = CUT;
            cut_message.endpoint1 = c;
            cut_message.endpoint2 = d;
            cut_message.start_tier = lct_response.weight;
            bcast(&cut_message, sizeof(UpdateMessage), tier_num+1);

            if (tier_num >= lct_response.weight)
                ett_nodes[c].cut(ett_nodes[d]);
        }

        // Tell all nodes above and including the current tier to add the new edge
        UpdateMessage link_message;
        link_message.type = LINK;
        link_message.endpoint1 = a;
        link_message.endpoint2 = b;
        link_message.start_tier = tier_num;
        bcast(&link_message, sizeof(UpdateMessage), tier_num+1);

        ett_nodes[a].link(ett_nodes[b]);
    }
}
