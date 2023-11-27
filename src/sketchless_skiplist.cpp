#include <cassert>
#include <xxhash.h>
#include "sketchless_skiplist.h"
#include "sketchless_euler_tour_tree.h"
#include <atomic>


double sketchless_height_factor;
long sketchless_skiplist_seed = time(NULL);

SketchlessSkipListNode::SketchlessSkipListNode(SketchlessEulerTourTree* node) : node(node) {}

void SketchlessSkipListNode::uninit_element(bool delete_bdry) {
	SketchlessSkipListNode* list_curr = this;
	SketchlessSkipListNode* list_prev;
	SketchlessSkipListNode* bdry_curr = this->left;
	SketchlessSkipListNode* bdry_prev;
	while (list_curr) {
		list_prev = list_curr;
		list_curr = list_prev->up;
		delete list_prev;
	}
	if (delete_bdry) {
		while (bdry_curr) {
			bdry_prev = bdry_curr;
			bdry_curr = bdry_prev->up;
			delete bdry_prev;
		}
	}
}

SketchlessSkipListNode* SketchlessSkipListNode::init_element(SketchlessEulerTourTree* node) {
	long seed = node->get_seed();
	// NOTE: WE SHOULD MAKE IT SO DIFFERENT SKIPLIST NODES FOR THE SAME ELEMENT CAN BE DIFFERENT HEIGHTS
	uint64_t element_height = sketchless_height_factor*__builtin_ctzll(XXH3_64bits_withSeed(&node->vertex, sizeof(node_id_t), sketchless_skiplist_seed))+1;
	SketchlessSkipListNode* list_node, *bdry_node, *list_prev, *bdry_prev;
	list_node = bdry_node = list_prev = bdry_prev = nullptr;
	// Add skiplist and boundary nodes up to the random height
	for (uint64_t i = 0; i < element_height; i++) {
		list_node = new SketchlessSkipListNode(node);
		bdry_node = new SketchlessSkipListNode(nullptr);
		list_node->left = bdry_node;
		bdry_node->right = list_node;
		if (list_prev) {
			list_node->down = list_prev;
			list_prev->up = list_node;
			list_prev->parent = list_node;
		}
		if (bdry_prev) {
			bdry_node->down = bdry_prev;
			bdry_prev->up = bdry_node;
			bdry_prev->parent = bdry_node;
		}
		list_prev = list_node;
		bdry_prev = bdry_node;
	}
	// Add one more boundary node at height+1
	SketchlessSkipListNode* root = new SketchlessSkipListNode(nullptr);
	root->down = bdry_prev;
	bdry_prev->up = root;
	bdry_prev->parent = root;
	list_prev->parent = root;
	return root->get_last();
}

SketchlessSkipListNode* SketchlessSkipListNode::get_parent() {
	// SkipListNode* curr = this;
	// while (curr && !curr->up) {
	// 	curr = curr->left;
	// }
	// return curr ? curr->up : nullptr;
	return parent;
}

SketchlessSkipListNode* SketchlessSkipListNode::get_root() {
	SketchlessSkipListNode* prev = nullptr;
	SketchlessSkipListNode* curr = this;
	while (curr) {
		prev = curr;
		curr = prev->get_parent();
	}
	return prev;
}

SketchlessSkipListNode* SketchlessSkipListNode::get_first() {
	// Go to the root first and then down to the first element, because if we start at some lower level
	// we may have to travel right a lot more on that level, takes log time instead of linear time
	SketchlessSkipListNode* prev = nullptr;
	SketchlessSkipListNode* curr = this->get_root();
	while (curr) {
		prev = curr;
		curr = prev->down;
	}
	return prev;
}

SketchlessSkipListNode* SketchlessSkipListNode::get_last() {
	// Go to the root first and then down to the last element, because if we start at some lower level
	// we may have to travel left a lot more on that level, takes log time instead of linear time
	SketchlessSkipListNode* prev = nullptr;
	SketchlessSkipListNode* curr = this->get_root();
	while (curr) {
		prev = curr;
		curr = prev->right ? prev->right : prev->down;
	}
	return prev;
}

std::set<SketchlessEulerTourTree*> SketchlessSkipListNode::get_component() {
	std::set<SketchlessEulerTourTree*> nodes;
	SketchlessSkipListNode* curr = this->get_first()->right; //Skip over the boundary node
	while (curr) {
		nodes.insert(curr->node);
		curr = curr->right;
	}
	return nodes;
}

void SketchlessSkipListNode::uninit_list() {
	SketchlessSkipListNode* curr = this->get_first();
	SketchlessSkipListNode* prev;
	while (curr) {
		prev = curr;
		curr = prev->right;
		prev->uninit_element(false);
	}
	prev->uninit_element(false);
}

SketchlessSkipListNode* SketchlessSkipListNode::join(SketchlessSkipListNode* left, SketchlessSkipListNode* right) {
	assert(left || right);
	if (!left) return right->get_root();
	if (!right) return left->get_root();

	SketchlessSkipListNode* l_curr = left->get_last();
	SketchlessSkipListNode* r_curr = right->get_first(); // this is the bottom boundary node
	SketchlessSkipListNode* r_first = r_curr->right;
	SketchlessSkipListNode* l_prev = nullptr;
	SketchlessSkipListNode* r_prev = nullptr;
	
	// Go up levels. link pointers, add aggregates
	while (l_curr && r_curr) {
		// Fix right pointer
		l_curr->right = r_curr->right; // skip over boundary node
		if (r_curr->right) r_curr->right->left = l_curr; // skip over boundary node, but to the left

		if (r_prev) delete r_prev; // Delete old boundary nodes
		l_prev = l_curr;
		r_prev = r_curr;
		l_curr = l_prev->get_parent();
		r_curr = r_prev->up;
	}

	// If left list was taller add the root agg in right to the rest in left
	while (l_curr) {
		l_prev = l_curr;
		l_curr = l_prev->get_parent();
	}

	// If right list was taller add new boundary nodes to left list
	if (r_curr) {
		while (r_curr) {
			l_curr = new SketchlessSkipListNode(nullptr);
			l_curr->down = l_prev;
			l_prev->up = l_curr;
			l_prev->parent = l_curr;
			l_curr->right = r_curr->right;
			if (r_curr->right) r_curr->right->left = l_curr;

			if (r_prev) delete r_prev; // Delete old boundary nodes
			l_prev = l_curr;
			r_prev = r_curr;
			r_curr = r_prev->up;
		}
	}
	delete r_prev;
	// Update parent pointers in right list
	while (r_first) {
		while (r_first && !r_first->up) {
			r_first->parent = r_first->left->parent;
			r_first = r_first->right;
		}
		if (r_first)
			r_first = r_first->up;
	}
	// Returns the root of the joined list
	return l_prev;
}

SketchlessSkipListNode* SketchlessSkipListNode::split_left(SketchlessSkipListNode* node) {
	assert(node && node->left && !node->down);
	// If just splitting off the boundary nodes do nothing instead
	if (!node->left->left) {
		return nullptr;
	}
	long seed = node->node->get_seed();
	// Construct new boundary nodes with correct aggregates for the right component
	// New aggs will be sum of all aggs on each level in the right path
	// Subtract those new aggregates from the "corners" of the left path
	// And unlink the nodes and link with the  new boundary nodes
	SketchlessSkipListNode* r_curr = node;
	SketchlessSkipListNode* l_curr = node->left;
	SketchlessSkipListNode* bdry = new SketchlessSkipListNode(nullptr);
	SketchlessSkipListNode* new_bdry;
	while (r_curr) {
		r_curr->left = bdry;
		bdry->right = r_curr;
		l_curr->right = nullptr;
		// Get next l_curr, r_curr, and bdry
		l_curr = l_curr->get_parent();
		new_bdry = new SketchlessSkipListNode(nullptr);
		while (r_curr && !r_curr->up) {
			r_curr->parent = new_bdry;
			r_curr = r_curr->right;
		}
		r_curr = r_curr ? r_curr->up : nullptr;
		new_bdry->down = bdry;
		bdry->up = new_bdry;
		bdry->parent = new_bdry;
		bdry = new_bdry;
	}
	// Subtract the final right agg from the rest of the aggs on left path
	SketchlessSkipListNode* l_prev = nullptr;
	while (l_curr) {
		l_prev  = l_curr;
		l_curr = l_curr->get_parent();
	}
	// Trim extra boundary nodes on the left list
	l_curr = l_prev->down;
	while (!l_curr->right) {
		delete l_prev;
		l_prev = l_curr;
		l_curr = l_prev->down;
	}
	l_prev->up = nullptr;
	l_prev->parent = nullptr;
	// Returns the root of left list
	return l_prev;
}

SketchlessSkipListNode* SketchlessSkipListNode::split_right(SketchlessSkipListNode* node) {
	assert(node);
	SketchlessSkipListNode* right = node->right;
	if (!right) return nullptr;
	SketchlessSkipListNode::split_left(right);
	return right->get_root();
}

SketchlessSkipListNode* SketchlessSkipListNode::next() {
	return this->right;
}
