#pragma once

#include "splay_tree.h"
#include "types.h"

class LinkCutTree;
class SplayTree;

class LinkCutNode{

  LinkCutNode* parent;
  LinkCutNode* dparent;
  LinkCutNode* left;
  LinkCutNode* right;
  
  //Each node contains the weight of the edge from itself to its parent in the represented tree  
  uint32_t edge_weight;

  public:
    void splay();
    void set_parent(LinkCutNode* parent);
    void set_dparent(LinkCutNode* dparent);
    void set_left(LinkCutNode* left);
    void set_right(LinkCutNode* right);
    void set_edge_weight(uint32_t weight);
};

class LinkCutTree{
  
  std::vector<LinkCutNode*> nodes;
  
  void expose(const LinkCutNode& v);
  void evert(const LinkCutNode& v);

  public:
    LinkCutTree(node_id_t num_nodes);
    ~LinkCutTree();

    // Given nodes v and w, link the trees containing v and w by adding the edge(v, w)
    void link(node_id_t v, node_id_t w, uint32_t weight);
    // Given nodes v and w, divide the tree containing v and w by deleting the edge(v, w)
    void cut(node_id_t v, node_id_t w);

    node_id_t find_root(node_id_t v);

    // Given node v and w return the edge with the maximum weight on the path from v to w and the weight itself
    std::pair<edge_id_t, uint32_t> path_aggregate(node_id_t v, node_id_t w);
};

