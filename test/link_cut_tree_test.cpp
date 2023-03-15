#include <gtest/gtest.h>
#include <iostream>
#include "link_cut_tree.h"

static void validate(LinkCutNode* v) {
    EXPECT_TRUE(v->get_parent() == nullptr
        || v->get_parent()->get_left() == v 
        || v->get_parent()->get_right() == v) << "Invalid node";
    EXPECT_TRUE(v->get_left() == nullptr
        || v->get_left()->get_parent() == v) << "Invalid node";
    EXPECT_TRUE(v->get_right() == nullptr
        || v->get_right()->get_parent() == v) << "Invalid node";
}

TEST(LinkCutTreeSuite, join_split_test) {
    // power of 2 node count
    int nodecount = 1024;
    std::vector<LinkCutNode> nodes;
    nodes.reserve(nodecount);
    for (int i = 0; i < nodecount; i++) {
        nodes.emplace_back();
    }
    // Join every 2,4,8,16... nodes
    for (int i = 2; i <= nodecount; i*=2) {
        for (int j = 0; j < nodecount; j+=i) {
            nodes[j].splay();
            nodes[j+i/2].splay();
            //std::cout << "Join nodes: " << &nodes[j] << " and " << &nodes[j+i/2] << "\n";
            LinkCutNode* p = LinkCutTree::join(&nodes[j], &nodes[j+i/2]);
            EXPECT_EQ(p->get_head(), &nodes[j]);
            EXPECT_EQ(p->get_tail(), &nodes[j+i-1]);
        }
        // Validate all nodes
        for (int i = 0; i < nodecount; i++) {
            validate(&nodes[i]);
        }
    }
    // Split Every ...16,8,4,2 nodes
    for (int i = nodecount; i > 1; i/=2) {
        for (int j = 0; j < nodecount; j+=i) {
            //std::cout << "Split on node: " << &nodes[j+i/2-1] << "\n";
            std::pair<LinkCutNode*, LinkCutNode*> paths = LinkCutTree::split(&nodes[j+i/2-1]);
            EXPECT_EQ(paths.first->get_head(), &nodes[j]);
            EXPECT_EQ(paths.first->get_tail(), &nodes[j+i/2-1]);
            EXPECT_EQ(paths.second->get_head(), &nodes[j+i/2]);
            EXPECT_EQ(paths.second->get_tail(), &nodes[j+i-1]);
        }
        // Validate all nodes
        for (int i = 0; i < nodecount; i++) {
            validate(&nodes[i]);
        }
    }
}

TEST(LinkCutTreeSuite, expose_simple_test) {
    int pathcount = 100;
    int nodesperpath = 100;
    std::vector<LinkCutNode> nodes;
    nodes.reserve(pathcount*nodesperpath);
    for (int i = 0; i < pathcount*nodesperpath; i++) {
        nodes.emplace_back();
    }
    // Link all the nodes in each path together
    for (int path = 0; path < pathcount; path++) {
        for (int node = 0; node < nodesperpath-1; node++) {
            nodes[path*nodesperpath+node].splay();
            LinkCutTree::join(&nodes[path*nodesperpath+node], &nodes[path*nodesperpath+node+1]);
        }
    }
    // Link all the paths together with dparent pointers half way up the previous path
    for (int path = 1; path < pathcount; path++) {
        nodes[path*nodesperpath].set_dparent(&nodes[path*nodesperpath-nodesperpath/2]);
    }
    // Call expose on the node half way up the bottom path
    LinkCutNode* p = LinkCutTree::expose(&nodes[pathcount*nodesperpath-nodesperpath/2]);

    // Validate all nodes
    for (int i = 0; i < pathcount*nodesperpath; i++) {
        validate(&nodes[i]);
    }
    // Validate head and tail of returned path
    EXPECT_EQ(p->get_head(), &nodes[0]);
    EXPECT_EQ(p->get_tail(), &nodes[pathcount*nodesperpath-nodesperpath/2]) << "Exposed node not tail of path";
    // Validate all dparent pointers
    for (int path = 0; path < pathcount; path++) {
        EXPECT_EQ(nodes[(path+1)*nodesperpath-nodesperpath/2+1].get_dparent(), &nodes[(path+1)*nodesperpath-nodesperpath/2]);
    }
}
