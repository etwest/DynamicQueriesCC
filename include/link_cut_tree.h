#pragma once

#include "splay_tree.h"
#include "types.h"

class LinkCutTree;
class SplayTree;

class LinkCutNode{
  LinkCutNode* parent;
  LinkCutNode* dparent;

  bool is_root;

  std::vector<LinkCutNode*> children;
  LinkCutNode* preferred_child;
  
  uint32_t edge_weight;


  public:
    void set_edge_weight(uint32_t tier);

};

class LinkCutTree{
  //need path aggregate
  //link
  //cut
  
  std::vector<LinkCutNode*> roots;
  
  void access(const LinkCutNode& v);
  uint32_t path_aggregate(const LinkCutNode& v);

  public:
    LinkCutTree(node_id_t num_nodes);

    // Given v a tree root and w a vertex in another tree, link the trees containing v and w by adding the edge(v, w), making w the parent of v
    void link(node_id_t v, node_id_t w);
    // Given node v not a tree root, divide the tree containing v into two trees by deleting the edge(v, parent(v))
    void cut(node_id_t v);

    node_id_t find_root(node_id_t v);

    // Given node v return the edge(a, b) with the maximum or minimum weight on the path from the tree root to v
    edge_id_t path_minimum(node_id_t v);
    edge_id_t path_maximum(node_id_t v);
};

