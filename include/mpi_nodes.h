#pragma once

#include "types.h"
#include <mpi.h>

#include "euler_tour_tree.h"
#include "link_cut_tree.h"
#include "mpi_functions.h"


#define MAX_INT (std::numeric_limits<int>::max())

enum TreeOperationType {
  NOT_ISOLATED=0, ISOLATED=1, EMPTY, LINK, CUT, LCT_QUERY
};

typedef struct {
  GraphUpdate update;
  bool end = false;
} UpdateMessage;

typedef struct {
  TreeOperationType type = EMPTY;
  node_id_t endpoint1 = 0;
  node_id_t endpoint2 = 0;
  uint32_t start_tier = 0;
} EttUpdateMessage;

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

typedef struct {
  uint32_t size1 = 0;
  uint32_t size2 = 0;
} GreedyRefreshMessage;

class InputNode {
  node_id_t num_nodes;
  uint32_t num_tiers;
  LinkCutTree link_cut_tree;
  UpdateMessage* update_buffer;
  int buffer_size;
  int buffer_capacity;
  bool* greedy_refresh_buffer;
  int* greedy_batch_buffer;
  void process_updates();
public:
  InputNode(node_id_t num_nodes, uint32_t num_tiers, int batch_size);
  ~InputNode();
  void update(GraphUpdate update);
  void process_all_updates();
  bool connectivity_query(node_id_t a, node_id_t b);
  std::vector<std::set<node_id_t>> cc_query();
  void end();
};

class TierNode {
  std::vector<EulerTourTree> ett_nodes;
  uint32_t tier_num;
  uint32_t num_tiers;
  int batch_size;
  UpdateMessage* update_buffer;
  GreedyRefreshMessage* this_sizes_buffer;
  GreedyRefreshMessage* next_sizes_buffer;
  SampleSketchRet* query_result_buffer;
  bool* split_revert_buffer;
  bool* greedy_refresh_buffer;
  int* greedy_batch_buffer;
  void update_tier(GraphUpdate update);
  void ett_update_tier(EttUpdateMessage message);
  void refresh_tier(RefreshMessage messsage);
public:
  TierNode(node_id_t num_nodes, uint32_t tier_num, uint32_t num_tiers, int batch_size);
  ~TierNode();
  void main();
};
