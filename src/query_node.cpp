#include "../include/graph_tiers.h"

QueryNode::QueryNode(node_id_t num_nodes, uint32_t num_tiers) : link_cut_tree(num_nodes), num_tiers(num_tiers) {this->num_nodes = num_nodes;};

void QueryNode::main() {
    while (true) {
        // Process a sketch update message or a connectivity query message
        StreamMessage stream_message;
        bcast(&stream_message, sizeof(StreamMessage), 0);
        if (stream_message.type == UPDATE) {
            if (stream_message.update.type == DELETE && link_cut_tree.has_edge(stream_message.update.edge.src, stream_message.update.edge.dst))
                link_cut_tree.cut(stream_message.update.edge.src, stream_message.update.edge.dst);
            MPI_Barrier(MPI_COMM_WORLD);
        } else if (stream_message.type == QUERY) {
            bool is_connected = link_cut_tree.find_root(stream_message.update.edge.src) == link_cut_tree.find_root(stream_message.update.edge.dst);
            MPI_Send(&is_connected, sizeof(bool), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
            continue;
        } else if (stream_message.type == CC_QUERY) {
            std::vector<std::set<node_id_t>> cc = link_cut_tree.get_cc();
            std::vector<node_id_t> cc_broadcast;
            cc_broadcast.reserve(num_nodes);
            for (node_id_t component_idx = 0; component_idx < cc.size(); component_idx++) {
                for (node_id_t vertex : cc[component_idx]) {
                    cc_broadcast[vertex] = component_idx;
                }
            }
            MPI_Send(&cc_broadcast[0], num_nodes*sizeof(node_id_t), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
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
                //std::cout << "RECEIVED LCT QUERY MESSAGE FROM " << rank << std::endl;

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
                    bcast(&update_message, sizeof(UpdateMessage), rank);
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
