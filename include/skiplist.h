#pragma once

#include <gtest/gtest.h>
#include "sketch.h"

class EulerTourTree;

class SkipListNode {

  SkipListNode* left = nullptr;
  SkipListNode* right = nullptr;
  SkipListNode* up = nullptr;
  SkipListNode* down = nullptr;

public:
  Sketch* sketch_agg;

  uint32_t size = 1;

  EulerTourTree* node;

  SkipListNode(EulerTourTree* node, long seed);
  ~SkipListNode();
  static SkipListNode* init_element(EulerTourTree* node);
  void uninit_element();

  // Returns the closest node on the next level up at or left of the current
  SkipListNode* get_parent();
  // Returns the top left root node of the skiplist
  SkipListNode* get_root();
  // Returns the bottom left boundary node of the skiplist
  SkipListNode* get_first();
  // Returns the bottom right node of the skiplist
  SkipListNode* get_last();

  // Return the aggregate size at the root of the list
  uint32_t get_list_size();
  // Return the aggregate sketch at the root of the list
  Sketch* get_list_aggregate();
  // Update all the aggregate sketches with the input vector from the current node to its root
  SkipListNode* update_path_agg(vec_t update_idx);
  // Add the given sketch to all aggregate sketches from the current node to its root
  SkipListNode* update_path_agg(Sketch* sketch);

  std::set<EulerTourTree*> get_component();

  // Returns the root of a new skiplist formed by joining the lists containing left and right
  static SkipListNode* join(SkipListNode* left, SkipListNode* right);
  template <typename... T>
  static SkipListNode* join(SkipListNode* head, T*... tail);
  // Returns the root of the left list after splitting to the left of the given node
  static SkipListNode* split_left(SkipListNode* node);
  // Returns the root of the right list after splitting to the right of the given node
  static SkipListNode* split_right(SkipListNode* node);

  bool isvalid();
  SkipListNode* next();
  int print_list();
};

template <typename... T>
SkipListNode* SkipListNode::join(SkipListNode* head, T*... tail) {
  return join(head, join(tail...));
}
