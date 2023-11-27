#pragma once
#include <iostream>
#include <unordered_map>
#include <set>

#include <sketchless_skiplist.h>
#include "types.h"

class SketchlessEulerTourTree {
  std::unordered_map<SketchlessEulerTourTree*, SketchlessSkipListNode*> edges;

  SketchlessSkipListNode* allowed_caller = nullptr;
  long seed = 0;

  SketchlessSkipListNode* make_edge(SketchlessEulerTourTree* other);
  void delete_edge(SketchlessEulerTourTree* other);

public:
  const node_id_t vertex = 0;
  const uint32_t tier = 0;

  SketchlessEulerTourTree(long seed, node_id_t vertex, uint32_t tier);
  SketchlessEulerTourTree(long seed);
  ~SketchlessEulerTourTree();
  bool link(SketchlessEulerTourTree& other);
  bool cut(SketchlessEulerTourTree& other);

  bool isvalid() const;

  SketchlessSkipListNode* get_root();

  uint32_t get_size();
  bool has_edge_to(SketchlessEulerTourTree* other);

  std::set<SketchlessEulerTourTree*> get_component();

  long get_seed() {return seed;};

  friend std::ostream& operator<<(std::ostream& os, const SketchlessEulerTourTree& ett);
};
