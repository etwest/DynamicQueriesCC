#include "../include/mpi_nodes.h"


long normal_refreshes = 0;

InputNode::InputNode(node_id_t num_nodes, uint32_t num_tiers, int batch_size) : num_nodes(num_nodes), num_tiers(num_tiers), link_cut_tree(num_nodes){
    update_buffer = (UpdateMessage*) malloc(sizeof(UpdateMessage)*(batch_size+1));
    buffer_capacity = batch_size+1;
    UpdateMessage msg;
    update_buffer[0] = msg;
    buffer_size = 1;
    split_revert_buffer = (int*) malloc(sizeof(int)*batch_size);
    history_size = 2*batch_size;
    for (int i=0; i<history_size; i++)
        isolation_history_queue.push(true);
    isolation_count = history_size;
};

InputNode::~InputNode() {
    free(update_buffer);
    free(split_revert_buffer);
}

void InputNode::update(GraphUpdate update) {
    UpdateMessage update_message;
    update_message.update = update;
    update_buffer[buffer_size++] = update_message;
    if (buffer_size == buffer_capacity)
        process_updates();
}

void InputNode::process_updates() {
    if (buffer_size == 1)
        return;
    uint32_t num_updates = buffer_size-1;
    // If less than 1/5 of the last updates are isolated use sliding window
    bool prev_strat = using_sliding_window;
    using_sliding_window = (isolation_count<history_size/5) ? true : false;
    if (using_sliding_window != prev_strat)
        std::cout << "SWITCHED TO " << (using_sliding_window ? "SLIDING WINDOW" : "NORMAL STRAT") << std::endl;
    // Broadcast the batch of updates to all nodes
    update_buffer[0].update.edge.src = num_updates;
    update_buffer[0].update.edge.dst = (int)using_sliding_window;
    bcast(&update_buffer[0], sizeof(UpdateMessage)*buffer_capacity, 0);
    // Do all the link cut tree cutting for things in the batch
    for (uint32_t i = 0; i < num_updates; i++) {
        GraphUpdate update = update_buffer[i+1].update;
        CANARY("INPUT");
        split_revert_buffer[i] = MAX_INT;
        unlikely_if (update.type == DELETE && link_cut_tree.has_edge(update.edge.src, update.edge.dst)) {
            split_revert_buffer[i] = link_cut_tree.get_edge_weight(update.edge.src, update.edge.dst);
            link_cut_tree.cut(update.edge.src, update.edge.dst);
        }
    }
    // Attempt to do the entire batch parallel with greedy refresh
    int isolated_update = MAX_INT;
    int minimum_isolated_update;
    allreduce(&isolated_update, &minimum_isolated_update);
    // Check for any isolation on any update on any tier
    if (minimum_isolated_update == MAX_INT) {
        buffer_size = 1;
        return;
    }
    // First undo all the link cut tree cuts we did after isolated update
    for (uint32_t update_idx = minimum_isolated_update; update_idx < num_updates+1; update_idx++) {
        GraphUpdate update = update_buffer[update_idx].update;
        // There could be a cut on a later update that needs to be rolled back
        unlikely_if (split_revert_buffer[update_idx-1] != MAX_INT)
            link_cut_tree.link(update.edge.src, update.edge.dst, split_revert_buffer[update_idx-1]);
    }
    // Update the isolation history
    for (uint32_t i = 1; i < minimum_isolated_update; i++) {
        isolation_count -= (int)isolation_history_queue.front();
        isolation_history_queue.pop();
        isolation_history_queue.push(true);
    }
    // ======================================================================================
    // =========================== PROCESS THE ISOLATED UPDATES =============================
    // ======================================================================================
    int end_update_idx = using_sliding_window ? minimum_isolated_update+1 : num_updates+1;
    for (int update_idx = minimum_isolated_update; update_idx < end_update_idx; update_idx++) {
        GraphUpdate update = update_buffer[update_idx].update;
        unlikely_if (update.type == DELETE && link_cut_tree.has_edge(update.edge.src, update.edge.dst))
            link_cut_tree.cut(update.edge.src, update.edge.dst);
        uint32_t start_tier = 0;
        normal_refreshes++;
        std::cout << "ISOLATED UPDATE: (" << update.edge.src << "," << update.edge.dst << ") "
	    << (update.type == DELETE ? "DELETE" : "INSERT") << std::endl;
        bool this_update_isolated = false;
        // Initiate the refresh sequence and receive all the broadcasts
        RefreshEndpoint e1, e2;
        e1.v = update.edge.src;
        e2.v = update.edge.dst;
        RefreshMessage refresh_message;
        refresh_message.endpoints = {e1, e2};
        MPI_Send(&refresh_message, sizeof(RefreshMessage), MPI_BYTE, start_tier+1, 0, MPI_COMM_WORLD);
        for (uint32_t tier = start_tier; tier < num_tiers; tier++) {
            int rank = tier + 1;
            if (tier != 0)
            for (auto endpoint : {0,1}) {
                std::ignore = endpoint;
                // Receive a broadcast to see if the current tier/endpoint is isolated or not
                EttUpdateMessage update_message;
                bcast(&update_message, sizeof(UpdateMessage), rank);
                if (update_message.type == NOT_ISOLATED)
                    continue;
                this_update_isolated = true;
                // Process a LCT query message first
                LctResponseMessage response_message;
                response_message.connected = link_cut_tree.find_root(update_message.endpoint1) == link_cut_tree.find_root(update_message.endpoint2);
                if (response_message.connected) {
                    std::pair<edge_id_t, uint32_t> max = link_cut_tree.path_aggregate(update_message.endpoint1, update_message.endpoint2);
                    response_message.cycle_edge = max.first;
                    response_message.weight = max.second;
                }
                MPI_Send(&response_message, sizeof(LctResponseMessage), MPI_BYTE, rank, 0, MPI_COMM_WORLD);

                // Then process two update broadcasts to potentially cut and link in the LCT
                for (auto broadcast : {0,1}) {
                    std::ignore = broadcast;
                    EttUpdateMessage update_message;
                    bcast(&update_message, sizeof(EttUpdateMessage), rank);
                    if (update_message.type == LINK) {
                        link_cut_tree.link(update_message.endpoint1, update_message.endpoint2, update_message.start_tier);
                        // std::cout << "LINK(" << update_message.endpoint1 << "," << update_message.endpoint2
                        // << ") ON TIERS >= " << update_message.start_tier << std::endl;
                        break;
                    } else if (update_message.type == CUT) {
                        link_cut_tree.cut(update_message.endpoint1, update_message.endpoint2);
                        // std::cout << "CUT(" << update_message.endpoint1 << "," << update_message.endpoint2
                        // << ") ON TIERS >= " << update_message.start_tier << std::endl;
                    }
                }
            }
        }
        isolation_count -= (int)isolation_history_queue.front();
        isolation_history_queue.pop();
        if (this_update_isolated) {
            isolation_history_queue.push(true);
            isolation_count += 1;
        } else
            isolation_history_queue.push(false);
    }
    // Shift the rest of the updates to the beginning of the buffer
    if (using_sliding_window) {
        for (int i = 0; i < buffer_size-minimum_isolated_update-1; i++)
            update_buffer[i+1] = update_buffer[minimum_isolated_update+i+1];
        buffer_size = buffer_size-minimum_isolated_update;
    } else {
        buffer_size = 1;
    }
}

void InputNode::process_all_updates() {
    while (buffer_size > 1)
        process_updates();
}

bool InputNode::connectivity_query(node_id_t a, node_id_t b) {
    process_all_updates();
    return link_cut_tree.find_root(a) == link_cut_tree.find_root(b);
}

std::vector<std::set<node_id_t>> InputNode::cc_query() {
    process_all_updates();
    return link_cut_tree.get_cc();
}

void InputNode::end() {
    process_all_updates();
    // Tell all nodes the stream is over
    update_buffer[0].end = true;
    bcast(update_buffer, sizeof(UpdateMessage)*buffer_capacity, 0);
     std::cout << "============= INPUT NODE =============" << std::endl;
     std::cout << "Normal refreshes: " << normal_refreshes << std::endl;
}
