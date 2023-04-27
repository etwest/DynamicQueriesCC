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
	uint64_t element_height = __builtin_ctzll(XXH3_64bits_withSeed(node->vertex))+1;
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
	return SkipList::get_last(root);
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
		l_curr->sketch_agg += r_curr->sketch_agg;
		if (r_prev) delete r_prev; // Delete old boundary nodes
		l_prev = l_curr;
		r_prev = r_curr;
		l_curr = l_prev->get_parent();
		r_curr = r_prev->get_parent();
	}
	// If left list was taller add the highest agg in right to the rest in left
	while (l_curr) {
		l_curr->sketch_agg += r_prev->sketch_agg;
		l_prev = l_curr;
		l_curr = l_prev->get_parent();
	}
	// If right list was taller add new boundary nodes to right list
	while (r_curr) {
		l_curr = new SkipListNode(nullptr, seed);
		l_curr->down = l_prev;
		l_prev->up = l_curr;
		l_curr->right = r_curr->right;
		if (r_curr->right) r_curr->right->left = l_curr;
		l_curr->sketch_agg += l_prev->sketch_agg + r_curr->sketch_agg;
		if (r_prev) delete r_prev; // Delete old boundary nodes
		l_prev = l_curr;
		r_prev = r_curr;
		r_curr = r_prev->get_parent();
	}
	delete r_prev;
	// Returns the root of the joined list
	return l_prev;
}
