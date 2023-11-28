#pragma once

#include "types.h"
#include <vector>
#include <atomic>

#include "euler_tour_tree.h"
#include "link_cut_tree.h"


// Global variables for performance testing
extern long lct_time;
extern long ett_time;
extern long ett_find_root;
extern long ett_get_agg;
extern long sketch_query;
extern long sketch_time;
extern long refresh_time;
extern long parallel_isolated_check;
extern long tiers_grown;
extern std::atomic<long> num_sketch_updates;
extern std::atomic<long> num_sketch_batches;

// maintains the tiers of the algorithm
// and the spanning forest of the entire graph
class GraphTiers {
  FRIEND_TEST(GraphTiersSuite, mini_correctness_test);
private:
  std::vector<EulerTourTree> ett;  // one ETT for each tier
  std::vector<SkipListNode*> root_nodes;
  LinkCutTree link_cut_tree;
  void refresh(GraphUpdate update);

  bool use_parallelism;

public:
  GraphTiers(node_id_t num_nodes, bool use_parallelism);
  ~GraphTiers();

  // apply an edge update
  void update(GraphUpdate update);

  // query for the connected components of the graph
  std::vector<std::set<node_id_t>> get_cc();

  // query for if a is connected to b
  bool is_connected(node_id_t a, node_id_t b);
};
