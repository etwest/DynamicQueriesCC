#include "types.h"

#include <vector>

#include "euler_tour_tree.h"
#include "link_cut_tree.h"

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
  // loop through each tier and update the sketches
  // at that tier
  void update(GraphUpdate update);

  // query for the connected components of the graph
  std::vector<std::vector<node_id_t>> get_cc();

  // query for if a is connected to b
  bool is_connected(node_id_t a, node_id_t b);
};
