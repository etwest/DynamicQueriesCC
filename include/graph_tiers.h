#pragma once

#include "types.h"
#include <vector>
#include <mpi.h>

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
extern long tiers_grown;

static edge_id_t vertices_to_edge(node_id_t a, node_id_t b) {
   return a<b ? (((edge_id_t)a)<<32) + ((edge_id_t)b) : (((edge_id_t)b)<<32) + ((edge_id_t)a);
};

enum OperationType {
  EMPTY, LINK, CUT, QUERY
};

typedef struct {
  OperationType type = EMPTY;
  node_id_t endpoint1 = 0;
  node_id_t endpoint2 = 0;
  uint32_t start_tier = 0;
} UpdateMessage;

typedef struct {
  OperationType type = EMPTY;
  node_id_t endpoint1 = 0;
  node_id_t endpoint2 = 0;
} LctQueryMessage;

typedef struct {
  bool connected = false;
  edge_id_t cycle_edge = 0;
  uint32_t weight = 0;
} LctResponseMessage;


typedef struct {
  node_id_t v = 0;
  uint32_t prev_tier_size = 0;
  SampleSketchRet sketch_query_result_type = ZERO;
  edge_id_t sketch_query_result = 0;
} RefreshMessage;

class TierNode {
  uint32_t tier_num;
  uint32_t num_tiers;
  std::vector<EulerTourTree> ett_nodes;
  void update_tier(GraphUpdate update);
  void ett_update_tier(UpdateMessage message);
  void refresh_tier(RefreshMessage endpoint1, RefreshMessage endpoint2);
public:
  TierNode(node_id_t num_nodes, uint32_t tier_num, uint32_t num_tiers);
  std::vector<std::set<node_id_t>> get_cc();
};

class QueryNode {
  LinkCutTree link_cut_tree;
  uint32_t num_tiers;
public:
  QueryNode(node_id_t num_nodes, uint32_t num_tiers);
  void main();
};
