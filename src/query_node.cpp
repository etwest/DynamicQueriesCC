#include "../include/graph_tiers.h"

QueryNode::QueryNode(node_id_t num_nodes, uint32_t num_tiers) : link_cut_tree(num_nodes), num_tiers(num_tiers) {};

void QueryNode::main() {
    while (true) {
        // Process a sketch update message or a connectivity query message
        StreamMessage stream_message;
        MPI_Bcast(&stream_message, sizeof(StreamMessage), MPI_BYTE, 0, MPI_COMM_WORLD);
        if (stream_message.type == UPDATE) {
            if (stream_message.update.type == DELETE)
                link_cut_tree.cut(stream_message.update.edge.src, stream_message.update.edge.dst);
        } else if (stream_message.type == QUERY) {
            bool is_connected = link_cut_tree.find_root(stream_message.update.edge.src) == link_cut_tree.find_root(stream_message.update.edge.dst);
            MPI_Send(&is_connected, sizeof(bool), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
            continue;
        } else {
            return;
        }
        // For each tier process LCT queries and LCT updates
        for (int tier = 0; tier < num_tiers; tier++) {
            int rank = tier + 1;
            for (int endpoint : {0,1}) {
                //Process a LCT query message first
                LctQueryMessage query_message;
                MPI_Recv(&query_message, sizeof(LctQueryMessage), MPI_BYTE, rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                if (query_message.type != EMPTY) {
                    LctResponseMessage response_message;
                    response_message.connected = link_cut_tree.find_root(query_message.endpoint1) == link_cut_tree.find_root(query_message.endpoint2);
                    if (response_message.connected) {
                        std::pair<edge_id_t, uint32_t> max = link_cut_tree.path_aggregate(query_message.endpoint1, query_message.endpoint2);
                        response_message.cycle_edge = max.first;
                        response_message.weight = max.second;
                    }
                    MPI_Send(&response_message, sizeof(LctResponseMessage), MPI_BYTE, rank, 0, MPI_COMM_WORLD);
                }

                // Then process two update broadcasts to potentially cut and link in the LCT
                for (int broadcast : {0,1}) {
                    UpdateMessage update_message;
                    MPI_Bcast(&update_message, sizeof(UpdateMessage), MPI_BYTE, rank, MPI_COMM_WORLD);
                    if (update_message.type == LINK) {
                        link_cut_tree.link(update_message.endpoint1, update_message.endpoint2, update_message.start_tier);
                    } else if (update_message.type == CUT) {
                        link_cut_tree.cut(update_message.endpoint1, update_message.endpoint2);
                    }
                }
            }
        }
    }
}