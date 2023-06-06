#include "../include/graph_tiers.h"

TierNode::TierNode(node_id_t num_nodes, uint32_t tier_num, uint32_t num_tiers) :
    tier_num(tier_num), num_tiers(num_tiers) {
    // Algorithm parameters
	vec_t sketch_len = (num_nodes*num_nodes);
	vec_t sketch_err = 10;
	uint32_t num_tiers = log2(num_nodes)/(log2(3)-1);
	int seed = time(NULL);

	// Configure the sketches globally
	Sketch::configure(sketch_len, sketch_err);

	// Initialize all the ETT node
    srand(seed);
    ett_nodes.reserve(num_nodes);
    for (node_id_t i = 0; i < num_nodes; ++i) {
        ett_nodes.emplace_back(seed, i, tier_num);
    }
}

void TierNode::main() {
    while (true) {
        // Process a sketch update message or a stream query message
        StreamMessage stream_message;
        MPI_Bcast(&stream_message, sizeof(StreamMessage), MPI_BYTE, 0, MPI_COMM_WORLD);
        if (stream_message.type == UPDATE) {
            update_tier(stream_message.update);
        } else if (stream_message.type == QUERY) {
            continue;
        } else {
            MPI_Finalize();
        }
        // Start the refreshing sequence
        for (int tier = 0; tier < tier_num; tier++) {
            int rank = tier + 1;
            // If this node's tier is the current tier process the refresh message from previous tier or input node
            if (tier == tier_num) {
                RefreshMessage refresh_message;
                if (tier_num == 0) {
                    MPI_Recv(&refresh_message, sizeof(RefreshMessage), MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                } else {
                    MPI_Recv(&refresh_message, sizeof(RefreshMessage), MPI_BYTE, tier_num-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                refresh_tier(refresh_message);
                continue;
            }
            // For every other tier just receive and perform update messages
            for (int endpoint : {0,1}) {
                for (int broadcast : {0,1}) {
                    UpdateMessage update_message;
                    MPI_Bcast(&update_message, sizeof(UpdateMessage), MPI_BYTE, rank, MPI_COMM_WORLD);
                    ett_update_tier(update_message);
                }
            }
        }
    }
}

void TierNode::update_tier(GraphUpdate update) {
    edge_id_t edge = vertices_to_edge(update.edge.src, update.edge.dst);
    ett_nodes[update.edge.src].update_sketch((vec_t)edge);
    ett_nodes[update.edge.dst].update_sketch((vec_t)edge);
    if (update.type == DELETE && ett_nodes[update.edge.src].has_edge_to(&ett_nodes[update.edge.dst])) {
        ett_nodes[update.edge.src].cut(ett_nodes[update.edge.dst]);
    }
}

void TierNode::ett_update_tier(UpdateMessage message) {
    if (message.type == LINK) {
        ett_nodes[message.endpoint1].link(ett_nodes[message.endpoint2]);
    } else if (message.type == CUT) {
        ett_nodes[message.endpoint1].cut(ett_nodes[message.endpoint2]);
    }
}

void TierNode::refresh_tier(RefreshMessage message) {
    for (RefreshEndpoint endpoint: {message.endpoints.first, message.endpoints.second}) {
        // Check if the tree containing this endpoint is isolated
        uint32_t prev_tier_size = endpoint.prev_tier_size;
        uint32_t this_tier_size = ett_nodes[endpoint.v].get_size();
        if (prev_tier_size != this_tier_size) {
            LctQueryMessage lct_query;
            lct_query.type = EMPTY;
            MPI_Send(&lct_query, sizeof(LctQueryMessage), MPI_BYTE, num_tiers+1, 0, MPI_COMM_WORLD);
            UpdateMessage update_message;
            update_message.type = EMPTY;
            MPI_Bcast(&update_message, sizeof(UpdateMessage), MPI_BYTE, tier_num, MPI_COMM_WORLD);
            MPI_Bcast(&update_message, sizeof(UpdateMessage), MPI_BYTE, tier_num, MPI_COMM_WORLD);
            continue;
        }

        // Check for new edge to eliminate isolation
        if (endpoint.sketch_query_result_type != GOOD) {
            LctQueryMessage lct_query;
            lct_query.type = EMPTY;
            MPI_Send(&lct_query, sizeof(LctQueryMessage), MPI_BYTE, num_tiers+1, 0, MPI_COMM_WORLD);
            UpdateMessage update_message;
            update_message.type = EMPTY;
            MPI_Bcast(&update_message, sizeof(UpdateMessage), MPI_BYTE, tier_num, MPI_COMM_WORLD);
            MPI_Bcast(&update_message, sizeof(UpdateMessage), MPI_BYTE, tier_num, MPI_COMM_WORLD);
            continue;
        }
        node_id_t a = (node_id_t)endpoint.sketch_query_result;
        node_id_t b = (node_id_t)(endpoint.sketch_query_result>>32);

        // Query LCT node to check if this new edge forms a cycle
        LctQueryMessage lct_query;
        lct_query.type = LCT_QUERY;
        lct_query.endpoint1 = a;
        lct_query.endpoint2 = b;
        MPI_Send(&lct_query, sizeof(LctQueryMessage), MPI_BYTE, num_tiers+1, 0, MPI_COMM_WORLD);

        LctResponseMessage lct_response;
        MPI_Recv(&lct_response, sizeof(LctResponseMessage), MPI_BYTE, num_tiers+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // If there is a cycle formed, tell all necessary nodes to delete that edge
        if (lct_response.connected) {
            node_id_t c = (node_id_t)lct_response.cycle_edge;
            node_id_t d = (node_id_t)(lct_response.cycle_edge>>32);

            UpdateMessage cut_message;
            cut_message.type = CUT;
            cut_message.endpoint1 = c;
            cut_message.endpoint2 = d;
            cut_message.start_tier = lct_response.weight;
            MPI_Bcast(&cut_message, sizeof(UpdateMessage), MPI_BYTE, tier_num, MPI_COMM_WORLD);

            if (tier_num >= lct_response.weight)
                ett_nodes[c].cut(ett_nodes[d]);
        } else {
            UpdateMessage cut_message;
            cut_message.type = EMPTY;
            MPI_Bcast(&cut_message, sizeof(UpdateMessage), MPI_BYTE, tier_num, MPI_COMM_WORLD);
        }

        // Tell all nodes above and including the current tier to add the new edge
        UpdateMessage link_message;
        link_message.type = LINK;
        link_message.endpoint1 = a;
        link_message.endpoint2 = b;
        link_message.start_tier = tier_num;
        MPI_Bcast(&link_message, sizeof(UpdateMessage), MPI_BYTE, tier_num, MPI_COMM_WORLD);

        ett_nodes[a].link(ett_nodes[b]);
    }
}

std::vector<std::set<node_id_t>> TierNode::get_cc() {
	std::vector<std::set<node_id_t>> cc;
	std::set<EulerTourTree*> visited;
	for (uint32_t i = 0; i < ett_nodes.size(); i++) {
		if (visited.find(&ett_nodes[i]) == visited.end()) {
			std::set<EulerTourTree*> pointer_component = ett_nodes[i].get_component();
			std::set<node_id_t> component;
			for (auto pointer : pointer_component) {
				component.insert(pointer-&ett_nodes[0]);
				visited.insert(pointer);
			}
			cc.push_back(component);
		}
	}
	return cc;
}
