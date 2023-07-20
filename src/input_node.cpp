#include "../include/graph_tiers.h"


InputNode::InputNode(node_id_t num_nodes, uint32_t num_tiers) : num_nodes(num_nodes), num_tiers(num_tiers) {
    link_cut_tree(num_nodes);
};

void InputNode::update(GraphUpdate update) {
    // Broadcast the update to all nodes for sketch updating
    StreamMessage stream_message;
    stream_message.type = UPDATE;
    stream_message.update = update;
    bcast(&stream_message, sizeof(StreamMessage), 0);
    // Try the greedy parallel refresh
    RefreshMessage empty_message;
    RefreshMessage* greedy_messages = (RefreshMessage*) malloc(sizeof(RefreshMessage)*(num_tiers+2));
    gather(&empty_message, sizeof(RefreshMessage), greedy_messages, sizeof(RefreshMessage), 0);
    UpdateMessage isolation_message;
    isolation_message.type = NOT_ISOLATED;
    for (uint32_t tier = 0; tier < num_tiers-1; tier++) {
        if (greedy_messages[tier].endpoints.first.prev_tier_size == greedy_messages[tier+1].endpoints.first.prev_tier_size
            && greedy_messages[tier].endpoints.first.sketch_query_result_type == GOOD) {
            isolation_message.type = ISOLATED;
            break;
        }
        if (greedy_messages[tier].endpoints.second.prev_tier_size == greedy_messages[tier+1].endpoints.second.prev_tier_size
            && greedy_messages[tier].endpoints.second.sketch_query_result_type == GOOD) {
            isolation_message.type = ISOLATED;
            break;
        }
    }
    bcast(&isolation_message, sizeof(UpdateMessage), 0);
    if (isolation_message.type == NOT_ISOLATED)
        return;
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

bool InputNode::connectivity_query(node_id_t a, node_id_t b) {
    // Send the CC query message to the query node and receive the response
    Edge e;
    e.src = a;
    e.dst = b;
    GraphUpdate update;
    update.edge = e;
    // StreamMessage stream_message;
    // stream_message.type = QUERY;
    // stream_message.update = update;
    // bcast(&stream_message, sizeof(StreamMessage), 0);
    bool is_connected = link_cut_tree.find_root(update.edge.src) == link_cut_tree.find_root(update.edge.dst);
    // MPI_Recv(&connected, sizeof(bool), MPI_BYTE, num_tiers+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    return is_connected;
}

std::vector<std::set<node_id_t>> InputNode::cc_query() {
    // Send the CC query message to the query node and receive the response
    // StreamMessage stream_message;
    // stream_message.type = CC_QUERY;
    
    // Query LCT for the cc
    std::vector<std::set<node_id_t>> cc = link_cut_tree.get_cc();
    std::vector<node_id_t> component_indices;
    component_indices.reserve(num_nodes);
    for (node_id_t component_idx = 0; component_idx < cc.size(); component_idx++) {
        for (node_id_t vertex : cc[component_idx]) {
            component_indices[vertex] = component_idx;
        }
    }

    // Convert from vector<node_id_t> to vector<set<node_id_t>>
    std::unordered_map<node_id_t, std::set<node_id_t>> component_map;
    for (node_id_t i = 0; i < num_nodes; i++) {
        if (component_map.find(component_indices[i]) != component_map.end()) {
            component_map[component_indices[i]].insert(i);
        }
        else {
            std::set<node_id_t> component = {i};
            component_map.insert({component_indices[i], component});
        }
    }

    std::vector<std::set<node_id_t>> cc(component_map.size());
    for (const auto& component : component_map) {
        cc[component.first] = component.second;
    }
    return cc;
}

void InputNode::end() {
    StreamMessage stream_message;
    stream_message.type = END;
    MPI_Bcast(&stream_message, sizeof(StreamMessage), MPI_BYTE, 0, MPI_COMM_WORLD);
}
