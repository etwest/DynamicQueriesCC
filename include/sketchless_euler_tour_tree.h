#pragma once
#include <iostream>
#include <unordered_map>
#include <set>

#include <sketchless_skiplist.h>
#include "types.h"

class SketchlessEulerTourNode {

  std::unordered_map<SketchlessEulerTourNode*, SketchlessSkipListNode*> edges;

  SketchlessSkipListNode* allowed_caller = nullptr;
  long seed = 0;

  SketchlessSkipListNode* make_edge(SketchlessEulerTourNode* other);
  void delete_edge(SketchlessEulerTourNode* other);

public:
  const node_id_t vertex = 0;
  const uint32_t tier = 0;

  SketchlessEulerTourNode(long seed, node_id_t vertex, uint32_t tier);
  SketchlessEulerTourNode(long seed);
  ~SketchlessEulerTourNode();
  bool link(SketchlessEulerTourNode& other);
  bool cut(SketchlessEulerTourNode& other);

  bool isvalid() const;

  SketchlessSkipListNode* get_root();

  bool has_edge_to(SketchlessEulerTourNode* other);

  std::set<SketchlessEulerTourNode*> get_component();

  long get_seed() {return seed;};

  friend std::ostream& operator<<(std::ostream& os, const SketchlessEulerTourNode& ett);
};

class SketchlessEulerTourTree {
  long seed = 0;
public:
  std::vector<SketchlessEulerTourNode> ett_nodes;

  SketchlessEulerTourTree(node_id_t num_nodes, uint32_t tier_num, int seed);
  
  void link(node_id_t u, node_id_t v);
  void cut(node_id_t u, node_id_t v);
  bool has_edge(node_id_t u, node_id_t v);
  SketchlessSkipListNode* get_root(node_id_t u);
  bool is_connected(node_id_t u, node_id_t v);
  std::vector<std::set<node_id_t>> cc_query();
};
