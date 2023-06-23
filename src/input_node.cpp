#include "../include/graph_tiers.h"


InputNode::InputNode(node_id_t num_nodes, uint32_t num_tiers) : num_nodes(num_nodes), num_tiers(num_tiers) {};

void InputNode::update(GraphUpdate update) {
    // Broadcast the update to all nodes for sketch updating
    StreamMessage stream_message;
    stream_message.type = UPDATE;
    stream_message.update = update;
    bcast(&stream_message, sizeof(StreamMessage), 0);
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
            for (auto broadcast : {0,1}) {
                std::ignore = broadcast;
                UpdateMessage update_message;
                bcast(&update_message, sizeof(UpdateMessage), rank);
                if (update_message.type == DONE) break;
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
    StreamMessage stream_message;
    stream_message.type = QUERY;
    stream_message.update = update;
    bcast(&stream_message, sizeof(StreamMessage), 0);
    bool connected;
    MPI_Recv(&connected, sizeof(bool), MPI_BYTE, num_tiers+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    return connected;
}

std::vector<std::set<node_id_t>> InputNode::cc_query() {
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
    StreamMessage stream_message;
    stream_message.type = END;
    MPI_Bcast(&stream_message, sizeof(StreamMessage), MPI_BYTE, 0, MPI_COMM_WORLD);
}
