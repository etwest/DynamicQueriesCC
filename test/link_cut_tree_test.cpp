#include <gtest/gtest.h>
#include <iostream>
#include "link_cut_tree.h"

static bool validate(LinkCutNode* v) {
    bool valid = true;
    if (!(v->get_parent() == nullptr || v->get_parent()->get_left() == v  || v->get_parent()->get_right() == v)) {
        valid = false;
        std::cout << "Node " << v << " invalid" << std::endl
        << "Parent's left: " << v->get_parent()->get_left()
        << " Parent's right: " << v->get_parent()->get_right() << std::endl;
    }
    if (!(v->get_left() == nullptr || v->get_left()->get_parent() == v)) {
        valid = false;
        std::cout << "Node " << v << " invalid" << std::endl
        << "Left's parent: " << v->get_left()->get_parent() << std::endl;
    }
    if (!(v->get_right() == nullptr || v->get_right()->get_parent() == v)) {
        valid = false;
        std::cout << "Node " << v << " invalid" << std::endl
        << "Rights's parent: " << v->get_right()->get_parent() << std::endl;
    }
    return valid;
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

TEST(LinkCutTreeSuite, random_links_and_cuts) {
    int nodecount = 1000;
    LinkCutTree lct(nodecount);
    int seed = time(NULL);
    // Link all nodes
    for (int i = 0; i < nodecount-1; i++) {
        lct.link(i,i+1, rand());
        ASSERT_TRUE(std::all_of(lct.nodes.begin(), lct.nodes.end(), [](auto& node){return validate(&node);}))
          << "One or more invalid nodes found" << std::endl;
    }
    // Cut every 10 nodes
    for (int i = 0; i < nodecount-1; i+=10) {
        lct.cut(i,i+1);
        ASSERT_TRUE(std::all_of(lct.nodes.begin(), lct.nodes.end(), [](auto& node){return validate(&node);}))
          << "One or more invalid nodes found" << std::endl;
    }
    // Do random links and cuts
    int n = 2000;
    std::cout << "Seeding random links and cuts test with " << seed << std::endl;
    srand(seed);
    for (int i = 0; i < n; i++) {
        node_id_t a = rand() % nodecount, b = rand() % nodecount;
        if (a != b) {
            if (rand() % 100 < 50 && lct.find_root(a) != lct.find_root(b)) {
                //std::cout << i << ": Linking " << a << " and " << b << std::endl;
                lct.link(a, b, rand());
            } else if (lct.find_root(a) == lct.find_root(b)) {
                //std::cout << i << ": Cutting " << a << " and " << b << std::endl;
                lct.cut(a, b);
            }
            ASSERT_TRUE(std::all_of(lct.nodes.begin(), lct.nodes.end(), [](auto& node){return validate(&node);}))
             << "One or more invalid nodes found" << std::endl;
        }
    }
}
