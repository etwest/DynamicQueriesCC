#include <vector>

#include "euler_tour_tree.h"

// maintains the tiers of the algorithm
// and the spanning forest of the entire graph
class GraphTiers {
 private:
  std::vector<std::vector<NodeData>> node_arr;  // for each tier, for each node

  void refresh(Edge new_edge);

 public:
  GraphTiers(node_id_t num_nodes);
  ~GraphTiers();

  // apply an edge update
  // loop through each tier and update the sketches
  // at that tier
  void update();

  // query for the connected components of the graph
  void get_cc();

  // query for if a is connected to b
  void is_connected(node_id_t a, node_id_t b);
};
