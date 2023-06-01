#include "../include/graph_tiers.h"

Tier::Tier(node_id_t num_nodes, node_id_t tier_number) :
    tier_number(tier_number) {
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
        ett_nodes.emplace_back(seed, i, tier_number);
    }
}

void Tier::update_tier(GraphUpdate update) {
    edge_id_t edge = vertices_to_edge(update.edge.src, update.edge.dst);
    ett_nodes[update.edge.src].update_sketch((vec_t)edge);
    ett_nodes[update.edge.dst].update_sketch((vec_t)edge);
    if (update.type == DELETE && ett_nodes[update.edge.src].has_edge_to(&ett_nodes[update.edge.dst])) {
        ett_nodes[update.edge.src].cut(ett_nodes[update.edge.dst]);
    }
}

void Tier::ett_update_tier(EttMessage message) {
    if (message.type == EMPTY) return;
    node_id_t a = (node_id_t)message.edge;
	node_id_t b = (node_id_t)(message.edge>>32);
    if (message.type == LINK) {
        ett_nodes[a].link(ett_nodes[b]);
    } else {
        ett_nodes[a].cut(ett_nodes[b]);
    }
}

void Tier::refresh_tier(RefreshMessage endpoint1, RefreshMessage endpoint2) {
    for (RefreshMessage message: {endpoint1, endpoint2}) {
        // Check if the tree containing this endpoint is isolated
        uint32_t prev_tier_size = message.prev_tier_size;
        uint32_t this_tier_size = ett_nodes[message.v].get_size();
        // Check for same size for isolated
        if (prev_tier_size != this_tier_size)
            continue;

        // Check for new edge to eliminate isolation
        if (message.query_result_type != GOOD)
            continue;

        node_id_t a = (node_id_t)message.query_result;
        node_id_t b = (node_id_t)(message.query_result>>32);

        // Check if a path exists between the edge's endpoints
        void* a_root = link_cut_tree.find_root(a);
        void* b_root = link_cut_tree.find_root(b);
        if (a_root == b_root) {
            // Find the maximum tier edge on the path and what tier it first appeared on
            std::pair<edge_id_t, uint32_t> max = link_cut_tree.path_aggregate(a,b);
            node_id_t c = (node_id_t)max.first;
            node_id_t d = (node_id_t)(max.first>>32);

            // Remove the maximum tier edge on all paths where it exists
            for (uint32_t i = max.second; i < ett_nodes.size(); i++) {
                ett_nodes[i][c].cut(ett_nodes[i][d]);
            }
            link_cut_tree.cut(c,d);
        }

        // Join the ETTs for the endpoints of the edge on all tiers above the current
        for (uint32_t i = tier+1; i < ett_nodes.size(); i++) {
            ett_nodes[i][a].link(ett_nodes[i][b]);
        }
        link_cut_tree.link(a,b, tier+1);
    }
}

std::vector<std::set<node_id_t>> Tier::get_cc() {
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
