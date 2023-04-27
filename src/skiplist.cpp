#include <cassert>
#include <xxhash.h>

#include "skiplist.h"
#include "euler_tour_tree.h"


SkipListNode::SkipListNode(EulerTourTree* node, long seed) :
	sketch_agg((Sketch*) ::operator new(Sketch::sketchSizeof())),
 	node(node) {
 	Sketch::makeSketch((char*)sketch_agg.get(), seed);
 }

SkipListNode* SkipListNode::init_element(EulerTourTree* node) {
	uint64_t element_height = __builtin_ctzll(XXH3_64bits_withSeed(node->vertex))+1;
	SkipListNode* list_node, *bdry_node, *list_prev, *bdry_prev;
	list_node = bdry_node = list_prev = bdry_prev = nullptr;
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
	SkipListNode* root = new SkipListNode(nullptr, node->get_seed());
	root->down = bdry_prev;
	bdry_prev->up = root;
	return SkipList::get_last(root);
}
