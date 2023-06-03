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

void TierNode::update_tier(GraphUpdate update) {
    edge_id_t edge = vertices_to_edge(update.edge.src, update.edge.dst);
    ett_nodes[update.edge.src].update_sketch((vec_t)edge);
    ett_nodes[update.edge.dst].update_sketch((vec_t)edge);
    if (update.type == DELETE && ett_nodes[update.edge.src].has_edge_to(&ett_nodes[update.edge.dst])) {
        ett_nodes[update.edge.src].cut(ett_nodes[update.edge.dst]);
    }
}

void TierNode::ett_update_tier(UpdateMessage message) {
    if (message.type == EMPTY) return;
    node_id_t a = message.endpoint1;
	node_id_t b = message.endpoint2;
    if (message.type == LINK) {
        ett_nodes[a].link(ett_nodes[b]);
    } else {
        ett_nodes[a].cut(ett_nodes[b]);
    }
}

void TierNode::refresh_tier(RefreshMessage endpoint1, RefreshMessage endpoint2) {
    for (RefreshMessage message: {endpoint1, endpoint2}) {
        // Check if the tree containing this endpoint is isolated
        uint32_t prev_tier_size = message.prev_tier_size;
        uint32_t this_tier_size = ett_nodes[message.v].get_size();
        if (prev_tier_size != this_tier_size)
            continue;

        // Check for new edge to eliminate isolation
        if (message.query_result_type != GOOD)
            continue;
        node_id_t a = (node_id_t)message.query_result;
        node_id_t b = (node_id_t)(message.query_result>>32);

        // Query LCT node to check if this new edge forms a cycle
        LctQueryMessage lct_query;
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
