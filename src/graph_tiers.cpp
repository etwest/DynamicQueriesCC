#include "../include/graph_tiers.h"

GraphTiers::GraphTiers(node_id_t num_nodes) {
	// Algorithm parameters
	vec_t sketch_len = log2(num_nodes);
	vec_t sketch_err = 100;
	node_id_t num_tiers = log2(num_nodes)/(log2(3)-1);
	int seed;

	// Configure the sketches globally
	Sketch::configure(sketch_len, sketch_err);

	// Initialize all the ETT nodes
	node_arr.reserve(num_nodes);
	for (node_id_t i = 0; i < num_nodes; i++) {
		node_arr[i].reserve(num_tiers);
		for (node_id_t j = 0; j < num_tiers; j++) {
			srand(seed);
			node_arr[i].emplace_back(seed);
		}
	}
}

GraphTiers::~GraphTiers() {}

void GraphTiers::update(GraphUpdate update) {
	edge_id_t edge = (((edge_id_t)update.edge.src)<<32) + ((edge_id_t)update.edge.dst);
	// Update the sketches of both endpoints of the edge in all tiers
	for (node_id_t i = 0; i < node_arr.size(); i++) {
		node_arr[update.edge.src][i].update_sketch((vec_t)edge);
		node_arr[update.edge.dst][i].update_sketch((vec_t)edge);
	}
	// Refresh the data structure
	refresh(update);
}

void GraphTiers::get_cc() {}

bool GraphTiers::is_connected(node_id_t a, node_id_t b) {}

void GraphTiers::refresh(GraphUpdate update) {}
