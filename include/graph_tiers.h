#pragma once

#include "types.h"
#include <vector>

#include "euler_tour_tree.h"
#include "link_cut_tree.h"

// Global variables for performance testing
#ifndef TIME_VARIABLES
#define TIME_VARIABLES
extern long lct_time;
extern long ett_time;
extern long ett_find_root;
extern long ett_get_agg;
extern long sketch_query;
extern long sketch_time;
extern long refresh_time;
extern long tiers_grown;
#endif

// maintains the tiers of the algorithm
// and the spanning forest of the entire graph
class GraphTiers {
private:
  std::vector<std::vector<EulerTourTree>> ett_nodes;  // for each tier, for each node
  LinkCutTree link_cut_tree;
  void refresh(GraphUpdate update);

public:
  GraphTiers(node_id_t num_nodes);
  ~GraphTiers();

  // apply an edge update
  void update(GraphUpdate update);

  // query for the connected components of the graph
  std::vector<std::set<node_id_t>> get_cc();

  // query for if a is connected to b
  bool is_connected(node_id_t a, node_id_t b);
};
