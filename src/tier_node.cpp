#include "../include/mpi_nodes.h"

long sketch_update_time = 0;
long sketch_query_time = 0;
long greedy_batch_time = 0;
long greedy_refresh_time = 0;
long normal_refresh_time = 0;

long greedy_batch_gather_time = 0;
long greedy_refresh_gather_time = 0;
long size_message_passing_time = 0;

TierNode::TierNode(node_id_t num_nodes, uint32_t tier_num, uint32_t num_tiers, int batch_size) :
    tier_num(tier_num), num_tiers(num_tiers), batch_size(batch_size) {
	// Initialize all the ETT node
    int seed = time(NULL);
    srand(seed);
    ett_nodes.reserve(num_nodes);
    for (node_id_t i = 0; i < num_nodes; ++i) {
        ett_nodes.emplace_back(seed, i, tier_num);
    }

    update_buffer = (StreamMessage*) malloc(sizeof(StreamMessage)*(batch_size+1));
    this_sizes_buffer = (GreedyRefreshMessage*) malloc(sizeof(GreedyRefreshMessage)*batch_size);
    next_sizes_buffer = (GreedyRefreshMessage*) malloc(sizeof(GreedyRefreshMessage)*batch_size);
    root_buffer = (SkipListNode**) malloc(sizeof(SkipListNode*)*batch_size*2);
    greedy_refresh_buffer = (bool*) malloc(sizeof(bool)*(num_tiers+1));
}

TierNode::~TierNode() {
    free(update_buffer);
    free(this_sizes_buffer);
    free(next_sizes_buffer);
    free(root_buffer);
    free(greedy_refresh_buffer);
}

void TierNode::main() {
    while (true) {
        // Receive a batch of updates and check if it is the end of stream
        bcast(&update_buffer[0], sizeof(StreamMessage)*(batch_size+1), 0);
        if (update_buffer[0].type == END) {
            std::cout << "============= TIER " << tier_num << " NODE =============" << std::endl;
            std::cout << "Greedy batch time (ms): " << greedy_batch_time/1000 << std::endl;
            std::cout << "\tSketch update time (ms): " << sketch_update_time/1000 << std::endl;
            std::cout << "\tSketch query time (ms): " << sketch_query_time/1000 << std::endl;
            std::cout << "\tSize message passing time (ms): " << size_message_passing_time/1000 << std::endl;
            std::cout << "\tGreedy gather time (ms): " << greedy_batch_gather_time/1000 << std::endl;
            std::cout << "Greedy refresh time (ms): " << greedy_refresh_time/1000 << std::endl;
            std::cout << "\tGreedy gather time (ms): " << greedy_refresh_gather_time/1000 << std::endl;
            std::cout << "Normal refresh time (ms): " << normal_refresh_time/1000 << std::endl;
            return;
        }
        // Do the greedy refresh check for all updates in the batch
        START(greedy_batch_timer);
        START(sketch_update_timer);
        bool any_update_isolated = false;
        for (uint32_t i = 0; i < update_buffer[0].update.edge.src-1; i++) {
            // Perform the sketch updating
            GraphUpdate update = update_buffer[i+1].update;
            edge_id_t edge = VERTICES_TO_EDGE(update.edge.src, update.edge.dst);
            unlikely_if (update.type == DELETE && ett_nodes[update.edge.src].has_edge_to(&ett_nodes[update.edge.dst])) {
                ett_nodes[update.edge.src].cut(ett_nodes[update.edge.dst]);
                any_update_isolated = true;
            }
            root_buffer[2*i] = ett_nodes[update.edge.src].update_sketch((vec_t)edge);
            root_buffer[2*i+1] = ett_nodes[update.edge.dst].update_sketch((vec_t)edge);
            // Prepare greedy batch size messages
            GreedyRefreshMessage this_sizes;
            this_sizes.size1 = root_buffer[2*i]->size;
            this_sizes.size2 = root_buffer[2*i+1]->size;
            this_sizes_buffer[i] = this_sizes;
        }
        STOP(sketch_update_time, sketch_update_timer);
        START(size_message_passing_timer);
        if (tier_num == 0) {
            MPI_Recv(next_sizes_buffer, batch_size*sizeof(GreedyRefreshMessage), MPI_BYTE, tier_num+2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else if (tier_num == num_tiers-1) {
            MPI_Send(this_sizes_buffer, batch_size*sizeof(GreedyRefreshMessage), MPI_BYTE, tier_num, 0, MPI_COMM_WORLD);
        } else if (tier_num%2 == 0) {
            MPI_Send(this_sizes_buffer, batch_size*sizeof(GreedyRefreshMessage), MPI_BYTE, tier_num, 0, MPI_COMM_WORLD);
            MPI_Recv(next_sizes_buffer, batch_size*sizeof(GreedyRefreshMessage), MPI_BYTE, tier_num+2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else if (tier_num%2 == 1) {
            MPI_Recv(next_sizes_buffer, batch_size*sizeof(GreedyRefreshMessage), MPI_BYTE, tier_num+2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(this_sizes_buffer, batch_size*sizeof(GreedyRefreshMessage), MPI_BYTE, tier_num, 0, MPI_COMM_WORLD);
        }
        STOP(size_message_passing_time, size_message_passing_timer);
        START(sketch_query_timer);
        if (!any_update_isolated) {
            for (uint32_t i = 0; i < update_buffer[0].update.edge.src-1; i++) {
                // Check if this tier is isolated for this update
                if (tier_num != num_tiers-1) {
                    if (this_sizes_buffer[i].size1 == next_sizes_buffer[i].size1) {
                        root_buffer[2*i]->process_updates();
                        if (root_buffer[2*i]->sketch_agg->query().second == GOOD) {
                            any_update_isolated = true;
                            break;
                        }
                    }
                    if (this_sizes_buffer[i].size2 == next_sizes_buffer[i].size2) {
                        root_buffer[2*i+1]->process_updates();
                        if (root_buffer[2*i+1]->sketch_agg->query().second == GOOD) {
                            any_update_isolated = true;
                            break;
                        }
                    }
                }
            }
        }
        STOP(sketch_query_time, sketch_query_timer);
        START(greedy_batch_gather_timer);
        allgather(&any_update_isolated, sizeof(bool), greedy_refresh_buffer, sizeof(bool));
        // Check for any isolation on any update on any tier
        int isolated_update = -1;
        for (uint32_t i = 0; i < num_tiers+1; i++) {
            unlikely_if (greedy_refresh_buffer[i]) {
                isolated_update = i;
                break;
            }
        }
        STOP(greedy_batch_gather_time, greedy_batch_gather_timer);
        STOP(greedy_batch_time, greedy_batch_timer);
        if (isolated_update < 0)
            continue;
        // Process all the updates in the batch normally
        for (uint32_t i = isolated_update; i < update_buffer[0].update.edge.src; i++) {
            GraphUpdate update = update_buffer[i].update;
            edge_id_t edge = VERTICES_TO_EDGE(update.edge.src, update.edge.dst);
            // Try the greedy parallel refresh
            START(greedy_refresh_timer);
            SkipListNode* root1 = ett_nodes[update.edge.src].get_root();
            SkipListNode* root2 = ett_nodes[update.edge.dst].get_root();
            GreedyRefreshMessage this_sizes;
            GreedyRefreshMessage next_sizes;
            this_sizes.size1 = root1->size;
            this_sizes.size2 = root2->size;
            next_sizes.size1 = root1->size;
            next_sizes.size2 = root2->size;
            if (tier_num == 0) {
                MPI_Recv(&next_sizes, sizeof(GreedyRefreshMessage), MPI_BYTE, tier_num+2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else if (tier_num == num_tiers-1) {
                MPI_Send(&this_sizes, sizeof(GreedyRefreshMessage), MPI_BYTE, tier_num, 0, MPI_COMM_WORLD);
            } else if (tier_num%2 == 0) {
                MPI_Send(&this_sizes, sizeof(GreedyRefreshMessage), MPI_BYTE, tier_num, 0, MPI_COMM_WORLD);
                MPI_Recv(&next_sizes, sizeof(GreedyRefreshMessage), MPI_BYTE, tier_num+2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else if (tier_num%2 == 1) {
                MPI_Recv(&next_sizes, sizeof(GreedyRefreshMessage), MPI_BYTE, tier_num+2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(&this_sizes, sizeof(GreedyRefreshMessage), MPI_BYTE, tier_num, 0, MPI_COMM_WORLD);
            }
            // Check if this tier is isolated
            bool isolated = false;
            if (tier_num != num_tiers-1) {
                if (this_sizes.size1 == next_sizes.size1) {
                    root1->process_updates();
                    if (root1->sketch_agg->query().second == GOOD)
                        isolated = true;
                }
                if (this_sizes.size2 == next_sizes.size2) {
                    root2->process_updates();
                    if (root2->sketch_agg->query().second == GOOD)
                        isolated = true;
                }
            }
            START(greedy_refresh_gather_timer);
            allgather(&isolated, sizeof(bool), greedy_refresh_buffer, sizeof(bool));
            STOP(greedy_refresh_gather_time, greedy_refresh_gather_timer);
            // Check for any isolation
            int tier_isolated = -1;
            for (uint32_t j = 1; j < num_tiers+1; j++) {
                unlikely_if (greedy_refresh_buffer[j]) {
                    tier_isolated = j-1;
                    break;
                }
            }
            STOP(greedy_refresh_time, greedy_refresh_timer);
            if (tier_isolated < 0)
                continue;
            uint32_t start_tier = tier_isolated;
            // Start the refreshing sequence
            START(normal_refresh_timer);
            for (uint32_t tier = start_tier; tier < num_tiers; tier++) {
                int rank = tier + 1;
                // If this node's tier is the current tier process the refresh message from previous tier or input node
                if (tier == tier_num) {
                    RefreshMessage refresh_message;
                    int source = (tier == start_tier) ? 0 : tier_num;
                    MPI_Recv(&refresh_message, sizeof(RefreshMessage), MPI_BYTE, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
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
