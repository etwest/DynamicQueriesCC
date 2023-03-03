#include "../include/graph_tiers.h"

GraphTiers::GraphTiers(node_id_t num_nodes) :
	link_cut_tree(num_nodes) {
	// Algorithm parameters
	vec_t sketch_len = num_nodes;
	vec_t sketch_err = 100;
	uint32_t num_tiers = log2(num_nodes)/(log2(3)-1);
	int seed;

	// Configure the sketches globally
	Sketch::configure(sketch_len, sketch_err);

	// Initialize all the ETT nodes
	node_arr.reserve(num_tiers);
	for (uint32_t i = 0; i < num_tiers; i++) {
		node_arr[i].reserve(num_nodes);
		srand(seed);
		for (node_id_t j = 0; j < num_nodes; j++) {
			node_arr[i].emplace_back(seed);
		}
	}
}

GraphTiers::~GraphTiers() {}

void GraphTiers::update(GraphUpdate update) {
	edge_id_t edge = (((edge_id_t)update.edge.src)<<32) + ((edge_id_t)update.edge.dst);
	// Update the sketches of both endpoints of the edge in all tiers
	for (uint32_t i = 0; i < node_arr.size(); i++) {
		node_arr[i][update.edge.src].update_sketch((vec_t)edge);
		node_arr[i][update.edge.dst].update_sketch((vec_t)edge);
	}
	// Refresh the data structure
	refresh(update);
}

void GraphTiers::get_cc() {}

bool GraphTiers::is_connected(node_id_t a, node_id_t b) {}

void GraphTiers::refresh(GraphUpdate update) {
	// For each tier for each endpoint of the edge
	for (uint32_t tier = 0; tier < node_arr.size()-1; tier++) {
		for (node_id_t v : {update.edge.src, update.edge.dst}) {
			std::shared_ptr<Sketch> ett_agg = node_arr[tier][v].get_aggregate();
			std::shared_ptr<Sketch> above_agg = node_arr[tier+1][v].get_aggregate();

			// Check if the tree containing this endpoint is isolated
			if (*ett_agg == *above_agg) {
				 std::pair<vec_t, SampleSketchRet> query_result = ett_agg->query();
				 if (query_result.second == GOOD) {
				 	edge_id_t edge = query_result.first;
					node_id_t a = (node_id_t)edge;
					node_id_t b = (node_id_t)(edge>>32);

					// Check if a path exists between the edge's endpoints
					if (link_cut_tree.find_root(a) == link_cut_tree.find_root(b)) {
						// Find the maximum tier edge on the path and what tier it first appeared on
						std::pair<edge_id_t, uint32_t> lct_max_a = link_cut_tree.path_aggregate(a);
						std::pair<edge_id_t, uint32_t> lct_max_b = link_cut_tree.path_aggregate(b);
						std::pair<edge_id_t, uint32_t> lct_max = (lct_max_a.second >= lct_max_b.second) ? lct_max_a : lct_max_b;
						node_id_t c = (node_id_t)lct_max.first;
						node_id_t d = (node_id_t)(lct_max.first>>32);

						// Remove the maximum tier edge on all paths where it exists
						for (uint32_t i = lct_max.second; i < node_arr.size(); i++) {
							node_arr[i][c].cut(node_arr[i][d]);
						}
						link_cut_tree.cut(c,d);
					}

					// Join the ETTs for the endpoints of the edge on all tiers above the current
					for (uint32_t i = tier+1; i < node_arr.size(); i++) {
						node_arr[i][a].link(node_arr[i][b]);
					}
					link_cut_tree.link(a,b);
				 }
			}
		}
	}
}
