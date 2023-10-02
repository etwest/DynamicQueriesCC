#include "../include/mpi_nodes.h"


long normal_refreshes = 0;
long update_count = 0;

InputNode::InputNode(node_id_t num_nodes, uint32_t num_tiers, int batch_size) : num_nodes(num_nodes), num_tiers(num_tiers), link_cut_tree(num_nodes){
    update_buffer = (UpdateMessage*) malloc(sizeof(UpdateMessage)*(batch_size+1));
    buffer_capacity = batch_size+1;
    UpdateMessage msg;
    update_buffer[0] = msg;
    buffer_size = 1;
    greedy_refresh_buffer = (bool*) malloc(sizeof(bool)*(num_tiers+1));
    greedy_batch_buffer = (int*) malloc(sizeof(int)*(num_tiers+1));
};

InputNode::~InputNode() {
    free(update_buffer);
    free(greedy_refresh_buffer);
    free(greedy_batch_buffer);
}

void InputNode::update(GraphUpdate update) {
    update_count++;
    if (update.type == DELETE)
        std::cout << "DELETE AT UPDATE " << update_count << std::endl;
    UpdateMessage update_message;
    update_message.update = update;
    update_message.update_number = update_count;
    update_buffer[buffer_size++] = update_message;
    if (buffer_size == buffer_capacity)
        process_updates();
}

void InputNode::process_updates() {
    // Broadcast the batch of updates to all nodes
    std::cout << "BATCH: [ ";
    for (int i = 0; i < buffer_capacity; i++)
        std::cout << update_buffer[i].update_number << " ";
    std::cout << "]" << std::endl;
    update_buffer[0].update.edge.src = buffer_size;
    bcast(update_buffer, sizeof(UpdateMessage)*buffer_capacity, 0);
    // Attempt to do the entire batch parallel with greedy refresh
    int isolated_update = MAX_INT;
    allgather(&isolated_update, sizeof(int), greedy_batch_buffer, sizeof(int));
    // Check for any isolated update
    int minimum_isolated_update = MAX_INT;
    for (uint32_t i = 0; i < num_tiers+1; i++)
        minimum_isolated_update = std::min(minimum_isolated_update, greedy_batch_buffer[i]);
    if (minimum_isolated_update == MAX_INT) {
        buffer_size = 1;
        return;
    }
    std::cout << "==================== ISOLATION AT INDEX " << minimum_isolated_update << "=====================" << std::endl;
    // Process the isolated update
    GraphUpdate update = update_buffer[minimum_isolated_update].update;
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
    uint32_t start_tier = 0;//std::max(0,tier_isolated-1);
    // Initiate the refresh sequence and receive all the broadcasts
    std::cout << "===============NORMAL REFRESHING==============" << std::endl;
    normal_refreshes++;
    RefreshEndpoint e1, e2;
    e1.v = update.edge.src;
    e2.v = update.edge.dst;
    RefreshMessage refresh_message;
    refresh_message.endpoints = {e1, e2};
    MPI_Send(&refresh_message, sizeof(RefreshMessage), MPI_BYTE, start_tier+1, 0, MPI_COMM_WORLD);
    for (uint32_t tier = start_tier; tier < num_tiers; tier++) {
        std::cout << "REFRESH TIER " << tier << std::endl;
        int rank = tier + 1;
        for (auto endpoint : {0,1}) {
            std::ignore = endpoint;
            // Receive a broadcast to see if the current tier/endpoint is isolated or not
            EttUpdateMessage update_message;
            bcast(&update_message, sizeof(EttUpdateMessage), rank);
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
                EttUpdateMessage update_message;
                bcast(&update_message, sizeof(EttUpdateMessage), rank);
                if (update_message.type == LINK) {
                    link_cut_tree.link(update_message.endpoint1, update_message.endpoint2, update_message.start_tier);
                    break;
                } else if (update_message.type == CUT) {
                    link_cut_tree.cut(update_message.endpoint1, update_message.endpoint2);
                }
            }
        }
    }
    std::cout << "=============FINISHED NORMAL REFRESH===============" << std::endl;
    // Shift the rest of the updates to the beginning of the buffer
    for (int i = 0; i < buffer_size-minimum_isolated_update-1; i++) {
        update_buffer[i+1] = update_buffer[minimum_isolated_update+i+1];
        update_buffer[i+1].updated_sketches = true;
    }
    buffer_size = buffer_size-minimum_isolated_update;
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
