#pragma once
#include <iostream>
#include <unordered_map>

#include <skiplist.h>


class EulerTourNode {
  FRIEND_TEST(EulerTourTreeSuite, random_links_and_cuts);
  FRIEND_TEST(EulerTourTreeSuite, get_aggregate);
  FRIEND_TEST(SkipListSuite, join_split_test);
  FRIEND_TEST(GraphTiersSuite, mini_correctness_test);
  
  std::unordered_map<EulerTourNode*, SkipListNode*> edges;

  SkipListNode* allowed_caller = nullptr;
  Sketch* temp_sketch = nullptr;
  long seed = 0;

  SkipListNode* make_edge(EulerTourNode* other, Sketch* temp_sketch);
  void delete_edge(EulerTourNode* other, Sketch* temp_sketch);

public:
  const node_id_t vertex = 0;
  const uint32_t tier = 0;

  EulerTourNode(long seed, node_id_t vertex, uint32_t tier);
  EulerTourNode(long seed);
  ~EulerTourNode();
  bool link(EulerTourNode& other, Sketch* temp_sketch);
  bool cut(EulerTourNode& other, Sketch* temp_sketch);

  bool isvalid() const;

  Sketch* get_sketch(SkipListNode* caller);
  SkipListNode* update_sketch(vec_t update_idx);

  SkipListNode* get_root();

  Sketch* get_aggregate();
  uint32_t get_size();
  bool has_edge_to(EulerTourNode* other);

  std::set<EulerTourNode*> get_component();

  long get_seed() {return seed;};

  friend std::ostream& operator<<(std::ostream& os, const EulerTourNode& ett);
};

class EulerTourTree {
  Sketch* temp_sketch;
public:
  std::vector<EulerTourNode> ett_nodes;
  
  EulerTourTree(node_id_t num_nodes, uint32_t tier_num);

  void link(node_id_t u, node_id_t v);
  void cut(node_id_t u, node_id_t v);
  bool has_edge(node_id_t u, node_id_t v);
  SkipListNode* update_sketch(node_id_t u, vec_t update_idx);
  SkipListNode* get_root(node_id_t u);
  Sketch* get_aggregate(node_id_t u);
  uint32_t get_size(node_id_t u);
};
