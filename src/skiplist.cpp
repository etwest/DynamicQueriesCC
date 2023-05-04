#include <cassert>
#include <xxhash.h>

#include "skiplist.h"
#include "euler_tour_tree.h"


SkipListNode::SkipListNode(EulerTourTree* node, long seed) :
	sketch_agg((Sketch*) ::operator new(Sketch::sketchSizeof())),
 	node(node) {
 	Sketch::makeSketch((char*)sketch_agg, seed);
 }

SkipListNode* SkipListNode::init_element(EulerTourTree* node) {
	uint64_t element_height = __builtin_ctzll(XXH3_64bits_withSeed(&node->vertex, sizeof(node_id_t), node->get_seed()))+1;
	SkipListNode* list_node, *bdry_node, *list_prev, *bdry_prev;
	list_node = bdry_node = list_prev = bdry_prev = nullptr;
	// Add skiplist and boundary nodes up to the random height
	for (uint64_t i = 0; i < element_height; i++) {
		list_node = new SkipListNode(node, node->get_seed());
		bdry_node = new SkipListNode(nullptr, node->get_seed());
		list_node->left = bdry_node;
		bdry_node->right = list_node;
		if (list_prev) {
			list_node->down = list_prev;
			list_prev->up = list_node;
		}
		if (bdry_prev) {
			bdry_node->down = bdry_prev;
			bdry_prev->up = bdry_node;
		}
	}
	// Add one more boundary node at height+1
	SkipListNode* root = new SkipListNode(nullptr, node->get_seed());
	root->down = bdry_prev;
	bdry_prev->up = root;
	return root->get_last();
}

SkipListNode* SkipListNode::get_parent() {
	SkipListNode* curr = this;
	while (curr && !curr->up) {
		curr = curr->left;
	}
	return curr ? curr->up : nullptr;
}

SkipListNode* SkipListNode::get_root() {
	SkipListNode* prev = nullptr;
	SkipListNode* curr = this;
	while (curr) {
		prev = curr;
		curr = prev->get_parent();
	}
	return prev;
}

SkipListNode* SkipListNode::get_first() {
	SkipListNode* prev = nullptr;
	SkipListNode* curr = this->get_root();
	while (curr) {
		prev = curr;
		curr = prev->down;
	}
	return prev;
}

SkipListNode* SkipListNode::get_last() {
	SkipListNode* prev = nullptr;
	SkipListNode* curr = this->get_root();
	while (curr) {
		prev = curr;
		curr = prev->right ? prev->right : prev->down;
	}
	return prev;
}

uint32_t SkipListNode::get_list_size() {
	return this->get_root()->size;
}

Sketch* SkipListNode::get_list_aggregate() {
	return this->get_root()->sketch_agg;
}

void SkipListNode::update_path_agg(vec_t update_idx) {
	SkipListNode* curr = this;
	while (curr) curr->sketch_agg->update(update_idx);
}

void SkipListNode::update_path_agg(Sketch* sketch) {
	SkipListNode* curr = this;
	while (curr) *curr->sketch_agg += *sketch;
}

SkipListNode* SkipListNode::join(SkipListNode* left, SkipListNode* right) {
	assert(left && right);
	SkipListNode* l_curr = left->get_last();
	SkipListNode* r_curr = right->get_first();
	long seed = l_curr->node->get_seed();
	SkipListNode* l_prev = nullptr;
	SkipListNode* r_prev = nullptr;
	// Go up levels. link pointers, add aggregates
	while (l_curr && r_curr) {
		l_curr->right = r_curr->right;
		if (r_curr->right) r_curr->right->left = l_curr;
		*l_curr->sketch_agg += *r_curr->sketch_agg;
		if (r_prev) delete r_prev; // Delete old boundary nodes
		l_prev = l_curr;
		r_prev = r_curr;
		l_curr = l_prev->get_parent();
		r_curr = r_prev->up;
	}
	// If left list was taller add the root agg in right to the rest in left
	while (l_curr) {
		*l_curr->sketch_agg += *r_prev->sketch_agg;
		l_prev = l_curr;
		l_curr = l_prev->get_parent();
	}
	// If right list was taller add new boundary nodes to left list
	while (r_curr) {
		l_curr = new SkipListNode(nullptr, seed);
		l_curr->down = l_prev;
		l_prev->up = l_curr;
		l_curr->right = r_curr->right;
		if (r_curr->right) r_curr->right->left = l_curr;
		*l_curr->sketch_agg += *l_prev->sketch_agg;
		*l_curr->sketch_agg += *r_curr->sketch_agg;
		if (r_prev) delete r_prev; // Delete old boundary nodes
		l_prev = l_curr;
		r_prev = r_curr;
		r_curr = r_prev->up;
	}
	delete r_prev;
	// Returns the root of the joined list
	return l_prev;
}

SkipListNode* SkipListNode::split_left(SkipListNode* node) {
	assert(node && node->left && !node->down);
	long seed = node->node->get_seed();
	// Construct new boundary nodes with correct aggregates for the right component
	// New aggs will be sum of all aggs on each level in the right path
	// Subtract those new aggregates from the "corners" of the left path
	// And unlink the nodes and link with the  new boundary nodes
	SkipListNode* r_curr = node;
	SkipListNode* l_curr = node->left;
	SkipListNode* bdry = new SkipListNode(nullptr, seed);
	SkipListNode* new_bdry;
	bool at_left_boundary = false;
	while (r_curr) {
		r_curr->left = bdry;
		bdry->right = r_curr;
		l_curr->right = nullptr;
		*l_curr->sketch_agg += *bdry->sketch_agg; // XOR addition same as subtraction
		// Get next l_curr, r_curr, and bdry
		l_curr = l_curr->get_parent();
		new_bdry = new SkipListNode(nullptr, seed);
		*new_bdry->sketch_agg += *bdry->sketch_agg;
		while (r_curr && !r_curr->up) {
			*new_bdry->sketch_agg += *r_curr->sketch_agg;
			r_curr = r_curr->right;
		}
		r_curr = r_curr ? r_curr->up : nullptr;
		new_bdry->down = bdry;
		bdry->up = new_bdry;
		bdry = new_bdry;
	}
	// Subtract the final right agg from the rest of the aggs on left path
	SkipListNode* l_prev;
	while (l_curr) {
		*l_curr->sketch_agg += *bdry->sketch_agg; // XOR addition same as subtraction
		l_prev  = l_curr;
		l_curr = l_curr->get_parent();
	}
	// Trim extra boundary nodes on the left list
	l_curr = l_prev->down;
	while (l_prev && (!l_curr || !l_curr->right)) {
		delete l_prev;
		l_prev = l_curr;
		l_curr = l_prev ? l_prev->down : nullptr;
	}
	if (l_curr) l_curr->up = nullptr;
	// Returns the root of left list
	return l_prev;
}

SkipListNode* SkipListNode::split_right(SkipListNode* node) {
	assert(node);
	SkipListNode* right = node->right;
	SkipListNode::split_left(right);
	return right->get_root();
}
