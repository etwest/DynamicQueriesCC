#pragma once
#include <iostream>
#include <unordered_map>

#include <skiplist.h>
#include "sketch/sketch_concept.h"
#include "sketch_interfacing.h"

#include <absl/container/flat_hash_map.h>



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
  SkipListNode<SketchClass>* update_sketch(const SketchClass &sketch);
  SkipListNode<SketchClass>* update_sketch_atomic(vec_t update_idx);
  SkipListNode<SketchClass>* update_sketch_atomic(const ColumnEntryDelta &delta);
  SkipListNode<SketchClass>* update_sketch_atomic(const ColumnEntryDeltas &deltas);
  
  // update just this node's sketch
  void update_sketch_noagg_atomic(const ColumnEntryDelta &delta);
  // void update_sketch_noagg_atomic(const SketchClass &sketch);
  
  //recompute the parent aggregates
  void recompute_aggregates_parallel();

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


using VectorContainer = std::vector<EulerTourNode<DefaultSketchColumn>>;
using HashmapContainer = absl::flat_hash_map<node_id_t, EulerTourNode<DefaultSketchColumn>*>;

template <typename SketchClass = DefaultSketchColumn, 
// typename Container = std::vector<EulerTourNode<SketchClass>>>
typename Container = absl::flat_hash_map<node_id_t, EulerTourNode<SketchClass>*>>
requires(SketchColumnConcept<SketchClass, vec_t>)
class EulerTourTree {
  SketchClass temp_sketch;
private:
  size_t seed;
  node_id_t max_num_nodes;
  uint32_t tier_num;
public:
  // std::vector<EulerTourNode<SketchClass>> ett_nodes;
  // absl::flat_hash_map<node_id_t, EulerTourNode<SketchClass>*> ett_nodes;
  Container ett_nodes;
  
  
  EulerTourTree(node_id_t max_num_nodes, uint32_t tier_num, int seed);

  EulerTourNode<SketchClass>& ett_node(node_id_t u) {
    if constexpr (std::is_same_v<Container, std::vector<EulerTourNode<SketchClass>>>) {
        assert(u < ett_nodes.size());
        return ett_nodes[u];
    } else {
        assert(ett_nodes.find(u) != ett_nodes.end());
        return *ett_nodes[u];
    }
  }
  
  void initialize_node(node_id_t u) {
    // no-op with vector implementation
    if constexpr (!std::is_same_v<Container, std::vector<EulerTourNode<SketchClass>>>) {
        // assert(ett_nodes.find(u) == ett_nodes.end());
        ett_nodes[u] = new EulerTourNode<SketchClass>(this->seed, u, this->tier_num);
    }
  };
  void uninitialize_node(node_id_t u) {
    // no-op with vector implementation
    if constexpr (!std::is_same_v<Container, std::vector<EulerTourNode<SketchClass>>>) {
        assert(ett_nodes.find(u) != ett_nodes.end());
        delete ett_nodes[u];
        // TODO - actually delete form ett
    }
  };
  
  void initialize_all_nodes() {
    for (node_id_t i = 0; i < max_num_nodes; ++i) {
        initialize_node(i);
    }
  };
  void initialize_all_nodes(node_id_t until) {
    assert(until <= max_num_nodes);
    for (node_id_t i = 0; i < until; ++i) {
        initialize_node(i);
    }
  }
  bool is_initialized(node_id_t u) {
    // no-op with vector implementation
    if constexpr (std::is_same_v<Container, std::vector<EulerTourNode<SketchClass>>>) {
        return true;
    } else {
        return ett_nodes.find(u) != ett_nodes.end();
    }
  };

  void link(node_id_t u, node_id_t v);
  void cut(node_id_t u, node_id_t v);
  bool has_edge(node_id_t u, node_id_t v);
  SkipListNode<SketchClass>* update_sketch(node_id_t u, vec_t update_idx);
  SkipListNode<SketchClass>* update_sketch(node_id_t u, const ColumnEntryDelta &delta);
  SkipListNode<SketchClass>* update_sketch(node_id_t u, const ColumnEntryDeltas &deltas);
  SkipListNode<SketchClass>* update_sketch(node_id_t u, const SketchClass &sketch);
  SkipListNode<SketchClass>* update_sketch_atomic(node_id_t u, vec_t update_idx);
  SkipListNode<SketchClass>* update_sketch_atomic(node_id_t u, const ColumnEntryDelta &delta);
  SkipListNode<SketchClass>* update_sketch_atomic(node_id_t u, const ColumnEntryDeltas &deltas);
  
  void update_sketch_noagg_atomic(const ColumnEntryDelta &delta);
  // void update_sketch_noagg_atomic(const SketchClass &sketch);
  
  //recompute the parent aggregates
  void recompute_aggregates_parallel();

  ColumnEntryDelta generate_entry_delta(node_id_t u, vec_t update) {
      // TODO - the specific node isnt actually meaningful here.
      return ett_node(u).generate_entry_delta(update);
  }

  std::pair<SkipListNode<SketchClass>*, SkipListNode<SketchClass>*> update_sketches(node_id_t u, node_id_t v, vec_t update_idx);
  SkipListNode<SketchClass>* get_root(node_id_t u);
  const SketchClass& get_aggregate(node_id_t u);
  uint32_t get_size(node_id_t u);
  uint32_t num_components() {
    std::set<void*> roots;
    for (node_id_t i = 0; i < ett_nodes.size(); ++i) {
      auto root = ett_node(i).get_root();
      roots.insert(root);
    }
    return roots.size();
  }
};
