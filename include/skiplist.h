#pragma once

#include <gtest/gtest.h>
#include "sketch.h"

class EulerTourTree;
class SkipList;

class SkipListNode {

  SkipListNode* left, *right, *up, *down;
  SkipListNode& get_parent();

  Sketch* sketch_agg;

  uint32_t size = 1;

public:
  EulerTourTree* node;

  SkipListNode(EulerTourTree* node, long seed);
  ~SkipListNode();
  static SkipListNode* init_element(EulerTourTree* node);

  uint32_t get_list_size();
  Sketch* get_list_aggregate();
  void update_path_agg(vec_t update_idx);
  void update_path_agg(Sketch* sketch);

  std::set<EulerTourTree*> get_component();

  bool isvalid();
  SkipListNode* next();
};

class SkipList {
public:
  static SkipListNode* join(SkipListNode* left, SkipListNode* right);
  template <typename... T>
  static SkipListNode* join(SkipListNode* head, T*... tail);
  static SkipListNode* split_left(SkipListNode* node);
  static SkipListNode* split_right(SkipListNode* node);
  static SkipListNode* get_last(SkipListNode* node);
};

template <typename... T>
SkipListNode* SkipList::join(SkipListNode* head, T*... tail) {
  return join(head, join(tail...));
}
