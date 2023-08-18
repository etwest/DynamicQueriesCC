#include <cassert>
#include <xxhash.h>
#include "skiplist.h"
#include "euler_tour_tree.h"
#include <atomic>


double height_factor;

std::atomic<long> num_sketch_updates(0);
std::atomic<long> num_sketch_batches(0);

SkipListNode::SkipListNode(EulerTourTree* node, long seed) :
	sketch_agg((Sketch*) ::operator new(Sketch::sketchSizeof())), node(node) {
	Sketch::makeSketch((char*)sketch_agg, seed);
	//update_buffer = (vec_t*) malloc(buffer_cap*sizeof(vec_t));
	buffer_capacity = buffer_cap;
}

SkipListNode::~SkipListNode() {
	::operator delete(sketch_agg, Sketch::sketchSizeof());
	//free(update_buffer);
}

void SkipListNode::uninit_element() {
	SkipListNode* list_curr = this;
	SkipListNode* list_prev;
	SkipListNode* bdry_curr = this->left;
	SkipListNode* bdry_prev;
	while (list_curr) {
		list_prev = list_curr;
		list_curr = list_prev->up;
		delete list_prev;
	}
	while (bdry_curr) {
		bdry_prev = bdry_curr;
		bdry_curr = bdry_prev->up;
		delete bdry_prev;
	}
}

SkipListNode* SkipListNode::init_element(EulerTourTree* node) {
	long seed = node->get_seed();
	// NOTE: WE SHOULD MAKE IT SO DIFFERENT SKIPLIST NODES FOR THE SAME ELEMENT CAN BE DIFFERENT HEIGHTS
	uint64_t element_height = height_factor*__builtin_ctzll(XXH3_64bits_withSeed(&node->vertex, sizeof(node_id_t), seed))+1;
	SkipListNode* list_node, *bdry_node, *list_prev, *bdry_prev;
	list_node = bdry_node = list_prev = bdry_prev = nullptr;
	// Add skiplist and boundary nodes up to the random height
	for (uint64_t i = 0; i < element_height; i++) {
		list_node = new SkipListNode(node, seed);
		bdry_node = new SkipListNode(nullptr, seed);
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
	SkipListNode* root = new SkipListNode(nullptr, seed);
	root->down = bdry_prev;
	bdry_prev->up = root;
	bdry_prev->parent = root;
	list_prev->parent = root;
	root->size = 2;
	return root->get_last();
}

SkipListNode* SkipListNode::get_parent() {
	// SkipListNode* curr = this;
	// while (curr && !curr->up) {
	// 	curr = curr->left;
	// }
	// return curr ? curr->up : nullptr;
	return parent;
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
	// Go to the root first and then down to the first element, because if we start at some lower level
	// we may have to travel right a lot more on that level, takes log time instead of linear time
	SkipListNode* prev = nullptr;
	SkipListNode* curr = this->get_root();
	while (curr) {
		prev = curr;
		curr = prev->down;
	}
	return prev;
}

SkipListNode* SkipListNode::get_last() {
	// Go to the root first and then down to the last element, because if we start at some lower level
	// we may have to travel left a lot more on that level, takes log time instead of linear time
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

void SkipListNode::update_agg(vec_t update_idx) {
	num_sketch_updates++;
	if (this->buffer_size == this->buffer_capacity)
		this->process_updates();
	this->update_buffer[this->buffer_size++] = update_idx;
}

void SkipListNode::process_updates() {
	num_sketch_batches++;
	for (int i = 0; i < buffer_size; i++)
		this->sketch_agg->update(update_buffer[i]);
	this->buffer_size = 0;
}

SkipListNode* SkipListNode::update_path_agg(vec_t update_idx) {
	SkipListNode* curr = this;
	SkipListNode* prev;
	while (curr) {
		curr->update_agg(update_idx);
		prev = curr;
		curr = prev->get_parent();
	}
	return prev;
}

SkipListNode* SkipListNode::update_path_agg(Sketch* sketch) {
	SkipListNode* curr = this;
	SkipListNode* prev;
	while (curr) {
		*curr->sketch_agg += *sketch;
		prev = curr;
		curr = prev->get_parent();
	}
	return prev;
}

std::set<EulerTourTree*> SkipListNode::get_component() {
	std::set<EulerTourTree*> nodes;
	SkipListNode* curr = this->get_first()->right; //Skip over the boundary node
	while (curr) {
		nodes.insert(curr->node);
		curr = curr->right;
	}
	return nodes;
}

SkipListNode* SkipListNode::join(SkipListNode* left, SkipListNode* right) {
	assert(left || right);
	if (!left) return right->get_root();
	if (!right) return left->get_root();

	long seed = left->sketch_agg->get_seed();
	assert(seed == right->sketch_agg->get_seed());

	SkipListNode* l_curr = left->get_last();
	SkipListNode* r_curr = right->get_first(); // this is the bottom boundary node
	SkipListNode* r_first = r_curr->right;
	SkipListNode* l_prev = nullptr;
	SkipListNode* r_prev = nullptr;
	
	// Go up levels. link pointers, add aggregates
	while (l_curr && r_curr) {
		// Fix right pointer and add agg
		l_curr->right = r_curr->right; // skip over boundary node
		if (r_curr->right) r_curr->right->left = l_curr; // skip over boundary node, but to the left
		r_curr->process_updates();
		*l_curr->sketch_agg += *r_curr->sketch_agg;
		l_curr->size += r_curr->size-1;

		if (r_prev) delete r_prev; // Delete old boundary nodes
		l_prev = l_curr;
		r_prev = r_curr;
		l_curr = l_prev->get_parent();
		r_curr = r_prev->up;
	}

	// If left list was taller add the root agg in right to the rest in left
	while (l_curr) {
		*l_curr->sketch_agg += *r_prev->sketch_agg;
		l_curr->size += r_prev->size-1;
		l_prev = l_curr;
		l_curr = l_prev->get_parent();
	}

	// If right list was taller add new boundary nodes to left list
	if (r_curr) {
		// Cache the left root to initialize the new boundary nodes
		Sketch* l_root_agg = (Sketch*) ::operator new(Sketch::sketchSizeof());
		Sketch::makeSketch((char*)l_root_agg, seed);
		l_prev->process_updates();
		*l_root_agg += *l_prev->sketch_agg;
		*l_root_agg += *r_prev->sketch_agg;
		uint32_t l_root_size = l_prev->size - (r_prev->size-1);
		while (r_curr) {
			l_curr = new SkipListNode(nullptr, seed);
			l_curr->down = l_prev;
			l_prev->up = l_curr;
			l_prev->parent = l_curr;
			l_curr->right = r_curr->right;
			if (r_curr->right) r_curr->right->left = l_curr;

			*l_curr->sketch_agg += *l_root_agg;
			l_curr->size = l_root_size;
			r_curr->process_updates();
			*l_curr->sketch_agg += *r_curr->sketch_agg;
			l_curr->size += r_curr->size-1;

			if (r_prev) delete r_prev; // Delete old boundary nodes
			l_prev = l_curr;
			r_prev = r_curr;
			r_curr = r_prev->up;
		}
		::operator delete(l_root_agg, Sketch::sketchSizeof());
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

SkipListNode* SkipListNode::split_left(SkipListNode* node) {
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
	SkipListNode* r_curr = node;
	SkipListNode* l_curr = node->left;
	SkipListNode* bdry = new SkipListNode(nullptr, seed);
	SkipListNode* new_bdry;
	while (r_curr) {
		r_curr->left = bdry;
		bdry->right = r_curr;
		l_curr->right = nullptr;
		*l_curr->sketch_agg += *bdry->sketch_agg; // XOR addition same as subtraction
		l_curr->size -= bdry->size-1;
		// Get next l_curr, r_curr, and bdry
		l_curr = l_curr->get_parent();
		new_bdry = new SkipListNode(nullptr, seed);
		*new_bdry->sketch_agg += *bdry->sketch_agg;
		new_bdry->size = bdry->size;
		while (r_curr && !r_curr->up) {
			r_curr->process_updates();
			*new_bdry->sketch_agg += *r_curr->sketch_agg;
			new_bdry->size += r_curr->size;
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
	SkipListNode* l_prev = nullptr;
	while (l_curr) {
		*l_curr->sketch_agg += *bdry->sketch_agg; // XOR addition same as subtraction
		l_curr->size -= bdry->size-1;
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

SkipListNode* SkipListNode::split_right(SkipListNode* node) {
	assert(node);
	SkipListNode* right = node->right;
	if (!right) return nullptr;
	SkipListNode::split_left(right);
	return right->get_root();
}

SkipListNode* SkipListNode::next() {
	return this->right;
}
