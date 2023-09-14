#include "../include/mpi_nodes.h"


long greedy_refresh_loop_time = 0;

long input_greedy_gather_time = 0;
long input_greedy_bcast_time = 0;

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
    START(input_greedy_gather_timer);
    bool isolated_message = false;
    allgather(&isolated_message, sizeof(bool), greedy_refresh_buffer, sizeof(bool));
    STOP(input_greedy_gather_time, input_greedy_gather_timer);
    // Check for any isolation
    bool any_update_isolated = false;
    START(greedy_refresh_loop_timer);
    for (uint32_t i = 0; i < num_tiers+1; i++) {
        unlikely_if (greedy_refresh_buffer[i]) {
            any_update_isolated = true;
            break;
        }
    }
    STOP(greedy_refresh_loop_time, greedy_refresh_loop_timer);
    if (!any_update_isolated) {
        update_buffer.clear();
        StreamMessage msg;
        update_buffer.push_back(msg);
        return;
    }
    // Process all those updates
    for (uint32_t i = 1; i < update_buffer.size(); i++) {
        GraphUpdate update = update_buffer[i].update;
        // Try the greedy parallel refresh
        START(input_greedy_gather_timer);
        bool isolated_message = false;
        allgather(&isolated_message, sizeof(bool), greedy_refresh_buffer, sizeof(bool));
        STOP(input_greedy_gather_time, input_greedy_gather_timer);
        // Check for any isolation
        bool any_tier_isolated = false;
        START(greedy_refresh_loop_timer);
        for (uint32_t i = 0; i < num_tiers+1; i++) {
            unlikely_if (greedy_refresh_buffer[i]) {
                any_tier_isolated = true;
                break;
            }
        }
        STOP(greedy_refresh_loop_time, greedy_refresh_loop_timer);
        if (!any_tier_isolated)
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
    std::cout << "Time in greedy refresh gather (ms): " << input_greedy_gather_time/1000 << std::endl;
    std::cout << "Time in greedy refresh loop (ms): " << greedy_refresh_loop_time/1000 << std::endl;
    std::cout << "Time in greedy refresh bcast (ms): " << input_greedy_bcast_time/1000 << std::endl;
}
