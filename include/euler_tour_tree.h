#pragma once
#include <iostream>
#include <unordered_map>

#include <skiplist.h>

class EulerTourTree {
  FRIEND_TEST(EulerTourTreeSuite, random_links_and_cuts);
  FRIEND_TEST(EulerTourTreeSuite, get_aggregate);
  FRIEND_TEST(SkipListSuite, join_split_test);
  FRIEND_TEST(GraphTiersSuite, mini_correctness_test);
  
  std::unordered_map<EulerTourTree*, SkipListNode*> edges;

  SkipListNode* allowed_caller = nullptr;
  Sketch* sketch = nullptr;
  long seed = 0;

  SkipListNode* make_edge(EulerTourTree* other);
  void delete_edge(EulerTourTree* other);

public:
  const node_id_t vertex = 0;
  const uint32_t tier = 0;

  EulerTourTree(long seed, node_id_t vertex, uint32_t tier);
  EulerTourTree(long seed);
  EulerTourTree(Sketch* sketch, long seed);
  bool link(EulerTourTree& other);
  bool cut(EulerTourTree& other);

  bool isvalid() const;

  Sketch* get_sketch(SkipListNode* caller);
  SkipListNode* update_sketch(vec_t update_idx);

  SkipListNode* get_root();

  Sketch* get_aggregate();
  uint32_t get_size();
  bool has_edge_to(EulerTourTree* other);

  std::set<EulerTourTree*> get_component();

  long get_seed() {return seed;};

  friend std::ostream& operator<<(std::ostream& os, const EulerTourTree& ett);
};
