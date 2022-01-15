#pragma once
#include "sketch.h"
#include "splay_tree.h"

class EulerTourTree;

struct NodeData {
  EulerTourTree *ett_ptrs[2];  // pointers to ETT aux tree (first and last)
  Sketch agg_sketch;        // aggregate sketch
};

//FIXME: Use sketch pointers here
//When updating a sketch, we DO NOT splay the node containing it in the aux_tree
// We simply call update on all parents as well
//This ensures that we don't have to rebuild logn aggregates

class EulerTourTree {
 private:
  // Tree representation splay tree, bbst, or skip-list
  SplayTree aux_tree;  // each node of tree points to a NodeData

 public:
  EulerTourTree(NodeData to_store);
  ~EulerTourTree();

  void link();
  void cut();

  void get_root();

  //void update_node_to_root_path(/*update id or something*/);

  //Sketch* subtree_aggregate() { return aux_tree.aggregate(); };
  //Sketch* path_aggregate(/*things*/) { return aux_tree.aggregate(); };
};
