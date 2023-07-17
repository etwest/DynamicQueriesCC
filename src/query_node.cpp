#include "../include/graph_tiers.h"


QueryNode::QueryNode(node_id_t num_nodes, uint32_t num_tiers, int batch_size) : link_cut_tree(num_nodes), num_nodes(num_nodes), num_tiers(num_tiers) {
    update_buffer.reserve(batch_size);
};

void QueryNode::main() {
    while (true) {
        // Receive a batch of updates and check if it is the end of stream
        bcast(&update_buffer[0], sizeof(StreamMessage)*update_buffer.capacity(), 0);
        if (update_buffer[0].type == END) {
            return;
        }
        // Process all the updates in the batch
        for (uint32_t i = 0; i < update_buffer.capacity(); i++) {
            if (update_buffer[i].update.type == DELETE && link_cut_tree.has_edge(update_buffer[i].update.edge.src, update_buffer[i].update.edge.dst))
                    link_cut_tree.cut(update_buffer[i].update.edge.src, update_buffer[i].update.edge.dst);
            // Participate in greedy refresh gather and bcast
            GreedyRefreshMessage empty_message;
            gather(&empty_message, sizeof(GreedyRefreshMessage), nullptr, 0, 0);
            UpdateMessage isolation_message;
            bcast(&isolation_message, sizeof(UpdateMessage), 0);
            if (isolation_message.type == NOT_ISOLATED)
                continue;
            // For each tier process LCT queries and LCT updates
            for (uint32_t tier = 0; tier < num_tiers; tier++) {
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
    }
}
