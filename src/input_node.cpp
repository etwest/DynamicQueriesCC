#include "../include/graph_tiers.h"


InputNode::InputNode(node_id_t num_nodes, uint32_t num_tiers, int batch_size) : num_nodes(num_nodes), num_tiers(num_tiers) {
    update_buffer.reserve(batch_size);
    greedy_refresh_buffer = (RefreshMessage*) malloc(sizeof(RefreshMessage)*(num_tiers+2));
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
        update_buffer.clear();
    }
}

void InputNode::process_updates() {
    // Broadcast the batch of updates to all nodes
    bcast(&update_buffer[0], sizeof(StreamMessage)*update_buffer.capacity(), 0);
    // Process all those updates
    for (uint32_t i = 0; i < update_buffer.capacity(); i++) {
        GraphUpdate update = update_buffer[i].update;
        // Try the greedy parallel refresh
        RefreshMessage empty_message;
        gather(&empty_message, sizeof(RefreshMessage), greedy_refresh_buffer, sizeof(RefreshMessage), 0);
        UpdateMessage isolation_message;
        isolation_message.type = NOT_ISOLATED;
        // TODO: make fast
        for (uint32_t tier = 0; tier < num_tiers-1; tier++) {
            unlikely_if (greedy_refresh_buffer[tier].endpoints.first.prev_tier_size == greedy_refresh_buffer[tier+1].endpoints.first.prev_tier_size
                && greedy_refresh_buffer[tier].endpoints.first.sketch_query_result_type == GOOD) {
                isolation_message.type = ISOLATED;
                break;
            }
            unlikely_if (greedy_refresh_buffer[tier].endpoints.second.prev_tier_size == greedy_refresh_buffer[tier+1].endpoints.second.prev_tier_size
                && greedy_refresh_buffer[tier].endpoints.second.sketch_query_result_type == GOOD) {
                isolation_message.type = ISOLATED;
                break;
            }
        }
        bcast(&isolation_message, sizeof(UpdateMessage), 0);
        if (isolation_message.type == NOT_ISOLATED)
            continue;
        // Initiate the refresh sequence and receive all the broadcasts
        RefreshEndpoint e1, e2;
        e1.v = update.edge.src;
        e2.v = update.edge.dst;
        RefreshMessage refresh_message;
        refresh_message.endpoints = {e1, e2};
        MPI_Send(&refresh_message, sizeof(RefreshMessage), MPI_BYTE, 1, 0, MPI_COMM_WORLD);
        for (uint32_t tier = 0; tier < num_tiers; tier++) {
            int rank = tier + 1;
            for (auto endpoint : {0,1}) {
                std::ignore = endpoint;
                // Receive a broadcast to see if the endpoint at the current tier is isolated or not
                UpdateMessage update_message;
                bcast(&update_message, sizeof(UpdateMessage), rank);
                if (update_message.type == NOT_ISOLATED) continue;
                // Get the two broadcasts for ett link or cut
                for (auto broadcast : {0,1}) {
                    std::ignore = broadcast;
                    UpdateMessage update_message;
                    bcast(&update_message, sizeof(UpdateMessage), rank);
                    if (update_message.type == LINK) break;
                }
            }
        }
    }
}

bool InputNode::connectivity_query(node_id_t a, node_id_t b) {
    process_updates();
    // Send the CC query message to the query node and receive the response
    Edge e;
    e.src = a;
    e.dst = b;
    GraphUpdate update;
    update.edge = e;
    StreamMessage stream_message;
    stream_message.type = QUERY;
    stream_message.update = update;
    bcast(&stream_message, sizeof(StreamMessage), 0);
    bool connected;
    MPI_Recv(&connected, sizeof(bool), MPI_BYTE, num_tiers+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    return connected;
}

std::vector<std::set<node_id_t>> InputNode::cc_query() {
    process_updates();
    // Send the CC query message to the query node and receive the response
    StreamMessage stream_message;
    stream_message.type = CC_QUERY;
    bcast(&stream_message, sizeof(StreamMessage), 0);
    std::vector<node_id_t> cc_broadcast;
    cc_broadcast.reserve(num_nodes);
    MPI_Recv(&cc_broadcast[0], num_nodes*sizeof(node_id_t), MPI_BYTE, num_tiers+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    // Convert from vector<node_id_t> to vector<set<node_id_t>>
    std::unordered_map<node_id_t, std::set<node_id_t>> component_map;
    for (node_id_t i = 0; i < num_nodes; i++) {
        if (component_map.find(cc_broadcast[i]) != component_map.end()) {
            component_map[cc_broadcast[i]].insert(i);
        }
        else {
            std::set<node_id_t> component = {i};
            component_map.insert({cc_broadcast[i], component});
        }
    }
    std::vector<std::set<node_id_t>> cc(component_map.size());
    for (const auto& component : component_map) {
        cc[component.first] = component.second;
    }
    return cc;
}

void InputNode::end() {
    process_updates();
    // Tell all nodes the stream is over
    StreamMessage stream_message;
    stream_message.type = END;
    update_buffer.push_back(stream_message);
    bcast(&update_buffer[0], sizeof(StreamMessage)*update_buffer.capacity(), 0);
}
