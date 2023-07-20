#pragma once

#include "types.h"
#include <vector>
#include <mpi.h>

#include "euler_tour_tree.h"
#include "link_cut_tree.h"
#include "mpi_functions.h"


enum StreamOperationType {
  UPDATE, QUERY, CC_QUERY, END
};

enum TreeOperationType {
  NOT_ISOLATED=0, ISOLATED=1, EMPTY, LINK, CUT, LCT_QUERY
};

typedef struct {
  StreamOperationType type = UPDATE;
  GraphUpdate update;
} StreamMessage;

typedef struct {
  TreeOperationType type = EMPTY;
  node_id_t endpoint1 = 0;
  node_id_t endpoint2 = 0;
  uint32_t start_tier = 0;
} UpdateMessage;

typedef struct {
  TreeOperationType type = EMPTY;
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
} RefreshEndpoint;

typedef struct {
  std::pair<RefreshEndpoint, RefreshEndpoint> endpoints;
} RefreshMessage;

class InputNode {
  node_id_t num_nodes;
  uint32_t num_tiers;
  LinkCutTree link_cut_tree;
public:
  InputNode(node_id_t num_nodes, uint32_t num_tiers);
  void update(GraphUpdate update);
  bool connectivity_query(node_id_t a, node_id_t b);
  std::vector<std::set<node_id_t>> cc_query();
  void end();
};

class TierNode {
  std::vector<EulerTourTree> ett_nodes;
  uint32_t tier_num;
  uint32_t num_tiers;
  void update_tier(GraphUpdate update);
  void ett_update_tier(UpdateMessage message);
  void refresh_tier(RefreshMessage messsage);
public:
  TierNode(node_id_t num_nodes, uint32_t tier_num, uint32_t num_tiers);
  void main();
};

class QueryNode {
  LinkCutTree link_cut_tree;
  node_id_t num_nodes;
  uint32_t num_tiers;
public:
  QueryNode(node_id_t num_nodes, uint32_t num_tiers);
  void main();
};
