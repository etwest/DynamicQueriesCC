#include "../include/mpi_nodes.h"


long normal_refreshes = 0;

InputNode::InputNode(node_id_t num_nodes, uint32_t num_tiers, int batch_size) : num_nodes(num_nodes), num_tiers(num_tiers), link_cut_tree(num_nodes){
    update_buffer.reserve(batch_size + 1);
    StreamMessage msg;
    update_buffer.push_back(msg);
    greedy_refresh_buffer = (bool*) malloc(sizeof(bool)*(num_tiers+1));
};

InputNode::~InputNode() {
    free(greedy_refresh_buffer);
}

void InputNode::update(GraphUpdate update) {
    StreamMessage stream_message;
    stream_message.type = UPDATE;
    stream_message.update = update;
    update_buffer.push_back(stream_message);
    if (update_buffer.size() == update_buffer.capacity()) {
        process_updates();
    }
}

void InputNode::process_updates() {
    // Broadcast the batch of updates to all nodes
    update_buffer[0].update.edge.src = update_buffer.size();
    bcast(&update_buffer[0], sizeof(StreamMessage)*update_buffer.capacity(), 0);
    // Attempt to do the entire batch parallel with greedy refresh
    for (uint32_t i = 0; i < update_buffer[0].update.edge.src-1; i++) {
        GraphUpdate update = update_buffer[i+1].update;
        if (update.type == DELETE && link_cut_tree.has_edge(update.edge.src, update.edge.dst))
            link_cut_tree.cut(update.edge.src, update.edge.dst);
    }
    bool isolated_message = false;
    allgather(&isolated_message, sizeof(bool), greedy_refresh_buffer, sizeof(bool));
    // Check for any isolation
    int isolated_update = -1;
    for (uint32_t i = 0; i < num_tiers+1; i++) {
        unlikely_if (greedy_refresh_buffer[i]) {
            isolated_update = i;
            break;
        }
    }
    if (isolated_update < 0) {
        update_buffer.clear();
        StreamMessage msg;
        update_buffer.push_back(msg);
        return;
    }
    // Process all those updates
    for (uint32_t i = isolated_update; i < update_buffer.size(); i++) {
        GraphUpdate update = update_buffer[i].update;
        // Try the greedy parallel refresh
        bool isolated_message = false;
        allgather(&isolated_message, sizeof(bool), greedy_refresh_buffer, sizeof(bool));
        // Check for any isolation
        int tier_isolated = -1;
        for (uint32_t j = 1; j < num_tiers+1; j++) {
            unlikely_if (greedy_refresh_buffer[j]) {
                tier_isolated = j-1;
                break;
            }
        }
        if (tier_isolated < 0)
            continue;
        uint32_t start_tier = tier_isolated;
        normal_refreshes++;
        // Initiate the refresh sequence and receive all the broadcasts
        RefreshEndpoint e1, e2;
        e1.v = update.edge.src;
        e2.v = update.edge.dst;
        RefreshMessage refresh_message;
        refresh_message.endpoints = {e1, e2};
        MPI_Send(&refresh_message, sizeof(RefreshMessage), MPI_BYTE, start_tier+1, 0, MPI_COMM_WORLD);
        for (uint32_t tier = start_tier; tier < num_tiers; tier++) {
            int rank = tier + 1;
            for (auto endpoint : {0,1}) {
                std::ignore = endpoint;
                // Receive a broadcast to see if the current tier/endpoint is isolated or not
                UpdateMessage update_message;
                bcast(&update_message, sizeof(UpdateMessage), rank);
                if (update_message.type == NOT_ISOLATED) continue;
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
                    UpdateMessage update_message;
                    bcast(&update_message, sizeof(UpdateMessage), rank);
                    if (update_message.type == LINK) {
                        link_cut_tree.link(update_message.endpoint1, update_message.endpoint2, update_message.start_tier);
                        break;
                    } else if (update_message.type == CUT) {
                        link_cut_tree.cut(update_message.endpoint1, update_message.endpoint2);
                    }
                }
            }
        }
    }
    update_buffer.clear();
    StreamMessage msg;
    update_buffer.push_back(msg);
}

bool InputNode::connectivity_query(node_id_t a, node_id_t b) {
    process_updates();
    return link_cut_tree.find_root(a) == link_cut_tree.find_root(b);
}

std::vector<std::set<node_id_t>> InputNode::cc_query() {
    process_updates();
    return link_cut_tree.get_cc();
}

void InputNode::end() {
    process_updates();
    // Tell all nodes the stream is over
    update_buffer[0].type = END;
    bcast(&update_buffer[0], sizeof(StreamMessage)*update_buffer.capacity(), 0);
    std::cout << "============= INPUT NODE =============" << std::endl;
    std::cout << "Normal Refreshes: " << normal_refreshes << std::endl;
}
