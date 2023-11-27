#pragma once

#include <set>

class SketchlessEulerTourTree;

extern long sketchless_skiplist_seed;
extern double sketchless_height_factor;

class SketchlessSkipListNode {

  SketchlessSkipListNode* left = nullptr;
  SketchlessSkipListNode* right = nullptr;
  SketchlessSkipListNode* up = nullptr;
  SketchlessSkipListNode* down = nullptr;
  // Store the first node to the left on the next level up
  SketchlessSkipListNode* parent = nullptr;

public:
  SketchlessEulerTourTree* node;

  SketchlessSkipListNode(SketchlessEulerTourTree* node);
  static SketchlessSkipListNode* init_element(SketchlessEulerTourTree* node);
  void uninit_element(bool delete_bdry);
  void uninit_list();

  // Returns the closest node on the next level up at or left of the current
  SketchlessSkipListNode* get_parent();
  // Returns the top left root node of the skiplist
  SketchlessSkipListNode* get_root();
  // Returns the bottom left boundary node of the skiplist
  SketchlessSkipListNode* get_first();
  // Returns the bottom right node of the skiplist
  SketchlessSkipListNode* get_last();

  // Return the aggregate size at the root of the list
  uint32_t get_list_size();

  std::set<SketchlessEulerTourTree*> get_component();

  // Returns the root of a new skiplist formed by joining the lists containing left and right
  static SketchlessSkipListNode* join(SketchlessSkipListNode* left, SketchlessSkipListNode* right);
  template <typename... T>
  static SketchlessSkipListNode* join(SketchlessSkipListNode* head, T*... tail);
  // Returns the root of the left list after splitting to the left of the given node
  static SketchlessSkipListNode* split_left(SketchlessSkipListNode* node);
  // Returns the root of the right list after splitting to the right of the given node
  static SketchlessSkipListNode* split_right(SketchlessSkipListNode* node);

  bool isvalid();
  SketchlessSkipListNode* next();
  int print_list();
};

template <typename... T>
SketchlessSkipListNode* SketchlessSkipListNode::join(SketchlessSkipListNode* head, T*... tail) {
  return join(head, join(tail...));
}
