#include <functional>
#include <gtest/gtest.h>
#include "skiplist.h"
#include "euler_tour_tree.h"

bool SkipListNode::isvalid() {
	bool valid = true;
	if (this->up && this->up->down != this) valid = false;
	if (this->down && this->down->up != this) valid = false;
	if (this->left && this->left->right != this) valid = false;
	if (this->right && this->right->left != this) valid = false;
    if (this->up && !this->up->isvalid()) valid = false;
    if (!this->get_parent() && this->right) valid = false;
	return valid;
}

int SkipListNode::print_list() {
    SkipListNode* curr = this->get_first();
    while (curr) {
        if (curr->node) std::cout << curr->node->vertex << ":\t";
        else std::cout << "-inf:\t";
        std::cout << curr <<  ": ";
        SkipListNode* currcurr = curr;
        while (currcurr) {
            std::cout << "O";
            currcurr = currcurr->up;
        }
        std::cout << std::endl;
        curr = curr->right;
    }
    return 0;
}

bool aggregate_correct(SkipListNode* node) {
    Sketch* naive_agg = new Sketch(sketch_len, node->node->get_seed(), 1, sketch_err);
    std::set<EulerTourNode*> component = node->get_component();
    for (auto ett_node : component) {
        naive_agg->update(ett_node->vertex);
    }
    node->get_root()->process_updates();
    Sketch* list_agg = node->get_list_aggregate();
    return *naive_agg == *list_agg;
}

TEST(SkipListSuite, join_split_test) {
    int num_elements = 1000;
    // Global sketch variables
    sketch_len = num_elements*num_elements;
    sketch_err = 100;

    long seed = time(NULL);
    srand(seed);
    EulerTourTree ett(num_elements, 0, seed);
    SkipListNode* nodes[num_elements];

    // Construct all of the ett_nodes and singleton SkipList nodes
    for (int i = 0; i < num_elements; i++) {
        ett.update_sketch(i, (vec_t)i);
        nodes[i] = ett.ett_nodes[i].allowed_caller;
    }

    // Link all the nodes two at a time, then link them all
    for (int i = 0; i < num_elements; i+=2) SkipListNode::join(nodes[i], nodes[i+1]);
    for (int i = 0; i < num_elements; i++) {
        ASSERT_TRUE(nodes[i]->isvalid());
        ASSERT_TRUE(aggregate_correct(nodes[i])) << "Node " << i << " agg incorrect";
    }
    for (int i = 0; i < num_elements-2; i+=2) SkipListNode::join(nodes[i], nodes[i+2]);
    for (int i = 0; i < num_elements; i++) {
        ASSERT_TRUE(nodes[i]->isvalid());
        ASSERT_TRUE(aggregate_correct(nodes[i])) << "Node " << i << " agg incorrect";
    }
    // Split all nodes into pairs, then split each pair
    for (int i = 0; i < num_elements-2; i+=2) SkipListNode::split_left(nodes[i+2]);
    for (int i = 0; i < num_elements; i++) {
        ASSERT_TRUE(nodes[i]->isvalid());
        ASSERT_TRUE(aggregate_correct(nodes[i])) << "Node " << i << " agg incorrect";
    }
    for (int i = 0; i < num_elements; i+=2) SkipListNode::split_left(nodes[i+1]);
    for (int i = 0; i < num_elements; i++) {
        ASSERT_TRUE(nodes[i]->isvalid());
        ASSERT_TRUE(aggregate_correct(nodes[i])) << "Node " << i << " agg incorrect";
    }
}
