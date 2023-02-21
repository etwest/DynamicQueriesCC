#pragma once

#include "splay_tree.h"

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
  void find_root(const LinkCutNode& v);

  public:
    // links nodes v and w
    bool link(const LinkCutNode& v, const LinkCutNode& w);
    LinkCutNode* cut(const LinkCutNode& v);
    uint32_t path_aggregate(const LinkCutNode& v);
};

