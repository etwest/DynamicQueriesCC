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
    int seed = time(NULL)*tier_num;
    srand(seed);
    std::cout << seed << std::endl;
    ett_nodes.reserve(num_nodes);
    for (node_id_t i = 0; i < num_nodes; ++i) {
        ett_nodes.emplace_back(seed, i, tier_num);
    }

    update_buffer = (UpdateMessage*) malloc(sizeof(UpdateMessage)*(batch_size+1));
    this_sizes_buffer = (GreedyRefreshMessage*) malloc(sizeof(GreedyRefreshMessage)*batch_size);
    next_sizes_buffer = (GreedyRefreshMessage*) malloc(sizeof(GreedyRefreshMessage)*batch_size);
    query_result_buffer = (SampleSketchRet*) malloc(sizeof(SampleSketchRet)*batch_size*2);
    split_revert_buffer = (bool*) malloc(sizeof(bool)*batch_size);
    greedy_refresh_buffer = (bool*) malloc(sizeof(bool)*(num_tiers+1));
    greedy_batch_buffer = (int*) malloc(sizeof(int)*(num_tiers+1));
}

TierNode::~TierNode() {
    free(update_buffer);
    free(this_sizes_buffer);
    free(next_sizes_buffer);
    free(query_result_buffer);
    free(split_revert_buffer);
    free(greedy_refresh_buffer);
    free(greedy_batch_buffer);
}

void TierNode::main() {
    while (true) {
        // Receive a batch of updates and check if it is the end of stream
        bcast(update_buffer, sizeof(UpdateMessage)*(batch_size+1), 0);
        if (update_buffer[0].end) {
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
        int first_cutting_update = MAX_INT;
        for (uint32_t i = 0; i < update_buffer[0].update.edge.src-1; i++) {
            // Perform the sketch updating or root finding
            GraphUpdate update = update_buffer[i+1].update;
            edge_id_t edge = VERTICES_TO_EDGE(update.edge.src, update.edge.dst);
            split_revert_buffer[i] = false;
            unlikely_if (update.type == DELETE && ett_nodes[update.edge.src].has_edge_to(&ett_nodes[update.edge.dst])) {
                ett_nodes[update.edge.src].cut(ett_nodes[update.edge.dst]);
                if (first_cutting_update == MAX_INT)
                    first_cutting_update = i+1;
                split_revert_buffer[i] = true;
            }
            SkipListNode* root1 = ett_nodes[update.edge.src].update_sketch((vec_t)edge);
            SkipListNode* root2 = ett_nodes[update.edge.dst].update_sketch((vec_t)edge);
            root1->process_updates();
            root1->sketch_agg->reset_queried();
            query_result_buffer[2*i] = root1->sketch_agg->query().second;
            root2->process_updates();
            root2->sketch_agg->reset_queried();
            query_result_buffer[2*i+1] = root2->sketch_agg->query().second;
            // Prepare greedy batch size messages
            GreedyRefreshMessage this_sizes;
            this_sizes.size1 = root1->size;
            this_sizes.size2 = root2->size;
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
        int isolated_update;
        // Check if this tier is isolated for each update
        isolated_update = MAX_INT;
        for (uint32_t i = 0; i < update_buffer[0].update.edge.src-1; i++) {
            // Check if this tier is isolated for this update
            if (tier_num != num_tiers-1) {
                if (this_sizes_buffer[i].size1 == next_sizes_buffer[i].size1)
                    if (query_result_buffer[2*i] == GOOD) {
                        isolated_update = i+1;
                        break;
                    }
                if (this_sizes_buffer[i].size2 == next_sizes_buffer[i].size2)
                    if (query_result_buffer[2*i+1] == GOOD) {
                        isolated_update = i+1;
                        break;
                    }
            }
        }
        isolated_update = std::min(isolated_update, first_cutting_update);
        STOP(sketch_query_time, sketch_query_timer);
        START(greedy_batch_gather_timer);
        allgather(&isolated_update, sizeof(int), greedy_batch_buffer, sizeof(int));
        // Check for any isolation on any update on any tier
        int minimum_isolated_update = MAX_INT;
        for (uint32_t i = 0; i < num_tiers+1; i++)
            minimum_isolated_update = std::min(minimum_isolated_update, greedy_batch_buffer[i]);
        STOP(greedy_batch_gather_time, greedy_batch_gather_timer);
        STOP(greedy_batch_time, greedy_batch_timer);
        if (minimum_isolated_update == MAX_INT)
            continue;
        // First undo all the sketch updates we did after isolated update
        for (uint32_t i = minimum_isolated_update; i < update_buffer[0].update.edge.src; i++) {
            GraphUpdate update = update_buffer[i].update;
            edge_id_t edge = VERTICES_TO_EDGE(update.edge.src, update.edge.dst);
            // How to correctly undo this part ??? note there could be a cut on a later update that actually needs to be rolled back
            unlikely_if (split_revert_buffer[i-1])
                ett_nodes[update.edge.src].link(ett_nodes[update.edge.dst]);
            ett_nodes[update.edge.src].update_sketch((vec_t)edge);
            ett_nodes[update.edge.dst].update_sketch((vec_t)edge);
        }
        // ======================================================================================
        // ============================ PROCESS THE ISOLATED UPDATE =============================
        // ======================================================================================
        GraphUpdate update = update_buffer[minimum_isolated_update].update;
        edge_id_t edge = VERTICES_TO_EDGE(update.edge.src, update.edge.dst);
        unlikely_if (update.type == DELETE && ett_nodes[update.edge.src].has_edge_to(&ett_nodes[update.edge.dst])) {
            ett_nodes[update.edge.src].cut(ett_nodes[update.edge.dst]);
        }
        SkipListNode* root1 = ett_nodes[update.edge.src].update_sketch((vec_t)edge);
        SkipListNode* root2 = ett_nodes[update.edge.dst].update_sketch((vec_t)edge);
        // Try the greedy parallel refresh
        START(greedy_refresh_timer);
        GreedyRefreshMessage this_sizes;
        GreedyRefreshMessage next_sizes;
        this_sizes.size1 = root1->size;
        this_sizes.size2 = root2->size;
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
                root1->sketch_agg->reset_queried();
                if (root1->sketch_agg->query().second == GOOD)
                    isolated = true;
            }
            if (this_sizes.size2 == next_sizes.size2) {
                root2->process_updates();
                root2->sketch_agg->reset_queried();
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
        //std::cout << "TIER NODE TI: " << tier_isolated << std::endl;
        STOP(greedy_refresh_time, greedy_refresh_timer);
        if (tier_isolated < 0)
            continue;
        uint32_t start_tier = 0;//tier_isolated;
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
                        ett_agg->reset_queried();
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
                EttUpdateMessage update_message;
                bcast(&update_message, sizeof(EttUpdateMessage), rank);
                if (update_message.type == NOT_ISOLATED) continue;
                // Get the two broadcasts and perform ett updates
                for (int broadcast : {0,1}) {
                    std::ignore = broadcast;
                    EttUpdateMessage update_message;
                    bcast(&update_message, sizeof(EttUpdateMessage), rank);
                    ett_update_tier(update_message);
                    if (update_message.type == LINK) break;
                }
            }
        }
        STOP(normal_refresh_time, normal_refresh_timer);
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

void TierNode::ett_update_tier(EttUpdateMessage message) {
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
        EttUpdateMessage update_message;
        update_message.type = (TreeOperationType)(!(prev_tier_size != this_tier_size || endpoint.sketch_query_result_type != GOOD));
        update_message.endpoint1 = a;
        update_message.endpoint2 = b;
        bcast(&update_message, sizeof(EttUpdateMessage), tier_num+1);
				
        if (update_message.type == NOT_ISOLATED)
            continue;
				
        // Query LCT node to check if this new edge forms a cycle
        LctResponseMessage lct_response;
        MPI_Recv(&lct_response, sizeof(LctResponseMessage), MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // If there is a cycle formed, tell all necessary nodes to delete that edge
        if (lct_response.connected) {
            node_id_t c = (node_id_t)lct_response.cycle_edge;
            node_id_t d = (node_id_t)(lct_response.cycle_edge>>32);

            EttUpdateMessage cut_message;
            cut_message.type = CUT;
            cut_message.endpoint1 = c;
            cut_message.endpoint2 = d;
            cut_message.start_tier = lct_response.weight;
            bcast(&cut_message, sizeof(EttUpdateMessage), tier_num+1);

            if (tier_num >= lct_response.weight)
                ett_nodes[c].cut(ett_nodes[d]);
        }

        // Tell all nodes above and including the current tier to add the new edge
        EttUpdateMessage link_message;
        link_message.type = LINK;
        link_message.endpoint1 = a;
        link_message.endpoint2 = b;
        link_message.start_tier = tier_num;
        bcast(&link_message, sizeof(EttUpdateMessage), tier_num+1);

        ett_nodes[a].link(ett_nodes[b]);
    }
}
