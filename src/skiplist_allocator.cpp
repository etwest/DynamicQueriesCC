#include "skiplist.h"


SkipListAllocator::SkipListAllocator() {}

SkipListAllocator::~SkipListAllocator() {
    for (void* block : block_list)
        free(block);
}

SkipListNode* SkipListAllocator::allocate_node(EulerTourNode* ett_node, long seed) {
    if (free_list.empty()) {
        void* block = malloc(sizeof(SkipListNode)*nodes_per_block);
        for (int i = 0; i < nodes_per_block; i++)
            free_list.push_back((SkipListNode*)(block+i*sizeof(SkipListNode)));
        block_list.push_back(block);
    }
    SkipListNode* address = free_list.front();
    // node->sketch_agg = new Sketch(sketch_len, seed, 1, sketch_err);
    // node->ett_node = ett_node;
    SkipListNode* new_node = new(address) SkipListNode(ett_node, seed);
    free_list.pop_front();
    return new_node;
}

void SkipListAllocator::free_node(SkipListNode* node) {
    delete node->sketch_agg;
    free_list.push_back(node);
}
