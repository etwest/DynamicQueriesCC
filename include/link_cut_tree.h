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

  public:
    LinkCutTree(node_id_t num_nodes);

    // Given nodes v and w, link the trees containing v and w by adding the edge(v, w)
    void link(node_id_t v, node_id_t w);
    // Given nodes v and w, divide the tree containing v and w by deleting the edge(v, w)
    void cut(node_id_t v, node_id_t w);

    node_id_t find_root(node_id_t v);

    // Given node v return the edge(a, b) with the maximum weight on the path from the tree root to v and the weight itself
    std::pair<edge_id_t, uint32_t> path_aggregate(node_id_t v);
};

