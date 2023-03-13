#pragma once

#include "splay_tree.h"
#include "types.h"

class LinkCutTree;
class SplayTree;

class LinkCutNode {

  LinkCutNode* parent;
  LinkCutNode* dparent;
  LinkCutNode* left;
  LinkCutNode* right;
  
  LinkCutNode* head;
  LinkCutNode* tail;

  //Each node contains the weight of the edge from itself to its parent in the represented tree  
  uint32_t edge_weight;
  //Maintain an aggregate maximum of the edge weights in the auxilliary tree
  uint32_t max;
  //Recompute the maximum just for this single node
  void rebuild_max();

  //Indicates that the meanings of left and right are reversed at all nodes in this subtree
  bool reversed;
  //Reverses all of appropriate nodes on the path from the root to this node
  void correct_reversals();

  void rotate_up();

  public:
    void splay();

    void link_left(LinkCutNode* left);
    void link_right(LinkCutNode* right);

    void set_parent(LinkCutNode* parent);
    void set_dparent(LinkCutNode* dparent);
    void set_edge_weight(uint32_t weight);
    void set_max(uint32_t weight);
    void set_reversed(bool reversed);

    LinkCutNode* get_left();
    LinkCutNode* get_right();
    LinkCutNode* get_parent();
    LinkCutNode* get_dparent();

    LinkCutNode* get_head();
    LinkCutNode* get_tail();
};

class LinkCutTree {
  
  std::vector<LinkCutNode*> nodes;
  
  LinkCutNode* splice(LinkCutNode* p);
  LinkCutNode* expose(LinkCutNode* v);
  void evert(LinkCutNode* v);

  public:
    LinkCutTree(node_id_t num_nodes);
    ~LinkCutTree();

    // Concatenate the paths with aux trees rooted at v and w and return the root of the combined aux tree
    LinkCutNode* join(LinkCutNode* v, LinkCutNode* w);
    // Split the aux tree of the path containing v right after v, and return the roots of the two new aux trees
    std::pair<LinkCutNode*, LinkCutNode*> split(LinkCutNode* v);
    
    // Given nodes v and w, link the trees containing v and w by adding the edge(v, w)
    void link(node_id_t v, node_id_t w, uint32_t weight);
    // Given nodes v and w, divide the tree containing v and w by deleting the edge(v, w)
    void cut(node_id_t v, node_id_t w);

    node_id_t find_root(node_id_t v);

    // Given node v and w return the edge with the maximum weight on the path from v to w and the weight itself
    std::pair<edge_id_t, uint32_t> path_aggregate(node_id_t v, node_id_t w);
};
