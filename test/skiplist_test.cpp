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

TEST(SkipListSuite, join_split_test) {
    int num_elements = 16;
    std::vector<EulerTourTree> ett_nodes;
    std::vector<SkipListNode*> nodes;
    long seed = time(NULL);
    srand(seed);
    // Construct all of the ett_nodes and singleton SkipList nodes
    for (int i = 0; i < num_elements; i++) {
        ett_nodes.emplace_back(seed, rand(), 0);
        nodes.emplace_back(ett_nodes[i].allowed_caller);
    }
    // Link all the nodes two at a time, then link them all
    for (int i = 0; i < num_elements; i+=2) SkipListNode::join(nodes[i], nodes[i+1]);
    for (int i = 0; i < num_elements; i++) ASSERT_TRUE(nodes[i]->isvalid());
    for (int i = 0; i < num_elements-2; i+=2) SkipListNode::join(nodes[i], nodes[i+2]);
    for (int i = 0; i < num_elements; i++) ASSERT_TRUE(nodes[i]->isvalid());
    // Split all nodes into pairs, then split each pair
    for (int i = 0; i < num_elements-2; i+=2) SkipListNode::split_left(nodes[i+2]);
    for (int i = 0; i < num_elements; i++) ASSERT_TRUE(nodes[i]->isvalid());
    for (int i = 0; i < num_elements; i+=2) SkipListNode::split_left(nodes[i+1]);
    for (int i = 0; i < num_elements; i++) ASSERT_TRUE(nodes[i]->isvalid());
}
