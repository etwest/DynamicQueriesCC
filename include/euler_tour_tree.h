#pragma once
#include <iostream>
#include <unordered_map>

#include <skiplist.h>
#include "sketch/sketch_concept.h"
#include "sketch_interfacing.h"



template <typename SketchClass> requires(SketchColumnConcept<SketchClass, vec_t>)
class EulerTourNode {
  FRIEND_TEST(EulerTourTreeSuite, random_links_and_cuts);
  FRIEND_TEST(EulerTourTreeSuite, get_aggregate);
  FRIEND_TEST(SkipListSuite, join_split_test);
  FRIEND_TEST(GraphTiersSuite, mini_correctness_test);
  
  std::unordered_map<EulerTourNode<SketchClass>*, SkipListNode<SketchClass>*> edges;

  long seed = 0;

  SkipListNode<SketchClass>* make_edge(EulerTourNode<SketchClass>* other, SketchClass &temp_sketch);
  SkipListNode<SketchClass>* make_edge(EulerTourNode<SketchClass>* other);
  void delete_edge(EulerTourNode<SketchClass>* other, SketchClass &temp_sketch);

public:
  const node_id_t vertex = 0;
  const uint32_t tier = 0;
  SkipListNode<SketchClass>* allowed_caller = nullptr;

  EulerTourNode(long seed, node_id_t vertex, uint32_t tier);
  EulerTourNode(long seed);
  ~EulerTourNode();
  bool link(EulerTourNode<SketchClass>& other, SketchClass &temp_sketch);
  bool cut(EulerTourNode<SketchClass>& other, SketchClass &temp_sketch);

  bool isvalid() const;

  SketchClass& get_sketch(SkipListNode<SketchClass>* caller);
  SkipListNode<SketchClass>* update_sketch(vec_t update_idx);
  SkipListNode<SketchClass>* update_sketch(const ColumnEntryDelta &delta);
  SkipListNode<SketchClass>* update_sketch(const ColumnEntryDeltas &deltas);
  SkipListNode<SketchClass>* update_sketch_atomic(vec_t update_idx);
  
  const ColumnEntryDelta generate_entry_delta(vec_t update) const {
    return this->allowed_caller->sketch_agg.generate_entry_delta(update);
  }

  SkipListNode<SketchClass>* get_root();

  const SketchClass& get_aggregate();
  uint32_t get_size();
  bool has_edge_to(EulerTourNode<SketchClass>* other);

  std::set<EulerTourNode<SketchClass>*> get_component();

  long get_seed() {return seed;};

  template <typename T> requires(SketchColumnConcept<T, vec_t>)
  friend std::ostream& operator<<(std::ostream& os, const EulerTourNode<T>& ett);
};

template <typename SketchClass = DefaultSketchColumn> requires(SketchColumnConcept<SketchClass, vec_t>)
class EulerTourTree {
  SketchClass temp_sketch;
public:
  std::vector<EulerTourNode<SketchClass>> ett_nodes;
  
  EulerTourTree(node_id_t num_nodes, uint32_t tier_num, int seed);

  void link(node_id_t u, node_id_t v);
  void cut(node_id_t u, node_id_t v);
  bool has_edge(node_id_t u, node_id_t v);
  SkipListNode<SketchClass>* update_sketch(node_id_t u, vec_t update_idx);
  SkipListNode<SketchClass>* update_sketch(node_id_t u, const ColumnEntryDelta &delta);
  SkipListNode<SketchClass>* update_sketch(node_id_t u, const ColumnEntryDeltas &deltas);
  SkipListNode<SketchClass>* update_sketch_atomic(node_id_t u, vec_t update_idx);
  ColumnEntryDelta generate_entry_delta(node_id_t u, vec_t update) const {
      // TODO - the specific node isnt actually meaningful here.
      return ett_nodes[u].generate_entry_delta(update);
  }

  std::pair<SkipListNode<SketchClass>*, SkipListNode<SketchClass>*> update_sketches(node_id_t u, node_id_t v, vec_t update_idx);
  SkipListNode<SketchClass>* get_root(node_id_t u);
  const SketchClass& get_aggregate(node_id_t u);
  uint32_t get_size(node_id_t u);
  uint32_t num_components() {
    std::set<void*> roots;
    for (node_id_t i = 0; i < ett_nodes.size(); ++i) {
      auto root = ett_nodes[i].get_root();
      roots.insert(root);
    }
    return roots.size();
  }
};
