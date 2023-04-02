#include "../include/graph_tiers.h"

GraphTiers::GraphTiers(node_id_t num_nodes) :
	link_cut_tree(num_nodes) {
	// Algorithm parameters
	vec_t sketch_len = num_nodes;
	vec_t sketch_err = 100;
	uint32_t num_tiers = log2(num_nodes)/(log2(3)-1);
	int seed = 0;

	// Configure the sketches globally
	Sketch::configure(sketch_len, sketch_err);

	// Initialize all the ETT node
	for (uint32_t i = 0; i < num_tiers; i++) {
		srand(seed);
		ett_nodes.emplace_back();
		for (node_id_t j = 0; j < num_nodes; j++) {
			ett_nodes[i].emplace_back(seed);
		}
	}
}

GraphTiers::~GraphTiers() {}

void GraphTiers::update(GraphUpdate update) {
	edge_id_t edge = (((edge_id_t)update.edge.src)<<32) + ((edge_id_t)update.edge.dst);
	// Update the sketches of both endpoints of the edge in all tiers
	for (uint32_t i = 0; i < ett_nodes.size(); i++) {
		ett_nodes[i][update.edge.src].update_sketch((vec_t)edge);
		ett_nodes[i][update.edge.dst].update_sketch((vec_t)edge);
	}
	// Refresh the data structure
	refresh(update);
}

std::vector<std::vector<node_id_t>> GraphTiers::get_cc() {
	std::vector<std::vector<node_id_t>> cc;
	std::set<EulerTourTree*> visited;
	int top = ett_nodes.size()-1;
	for (uint32_t i = 0; i < ett_nodes[top].size(); i++) {
		if (visited.find(&ett_nodes[top][i]) == visited.end()) {
			std::set<EulerTourTree*> pointer_component = ett_nodes[top][i].get_component();
			std::vector<node_id_t> component;
			for (auto pointer : pointer_component) {
				component.push_back(pointer-&ett_nodes[top][0]);
				visited.insert(pointer);
			}
			cc.push_back(component);
		}
	}
	return cc;
}

bool GraphTiers::is_connected(node_id_t a, node_id_t b) {
	return this->link_cut_tree.find_root(a) == this->link_cut_tree.find_root(b);
}

void GraphTiers::refresh(GraphUpdate update) {
	// For each tier for each endpoint of the edge
	for (uint32_t tier = 0; tier < ett_nodes.size()-1; tier++) {
		for (node_id_t v : {update.edge.src, update.edge.dst}) {
			std::shared_ptr<Sketch> ett_agg = ett_nodes[tier][v].get_aggregate();

			// Check if the tree containing this endpoint is isolated
			if (ett_nodes[tier][v].get_size() == ett_nodes[tier+1][v].get_size()) {
				 std::pair<vec_t, SampleSketchRet> query_result = ett_agg->query();
				 if (query_result.second == GOOD) {
				 	edge_id_t edge = query_result.first;
					node_id_t a = (node_id_t)edge;
					node_id_t b = (node_id_t)(edge>>32);

					// Check if a path exists between the edge's endpoints
					if (link_cut_tree.find_root(a) == link_cut_tree.find_root(b)) {
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
		}
	}
}
