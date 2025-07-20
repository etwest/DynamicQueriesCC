
#include "../include/batch_tiers.h"
#include "util.h"
#include <random>
#include <atomic>

// // #define CANARY(X) do {if (update.edge.src == 1784 && update.edge.dst == 4420) { std::cout << __FILE__ << ":" << __LINE__ << " says " << X << std::endl;}} while (false)
// #define CANARY(X) ;
// // #define ENDPOINT_CANARY(X, src, dst) do {if ((src == 7781 || dst == 7781)) {std::cout << __FILE__ << ":" << __LINE__ << " says " << X << " " << src << " " << dst << std::endl;}} while (false)
// #define ENDPOINT_CANARY(X, src, dst) ;

// long lct_time = 0;
// long ett_time = 0;
// long ett_find_root = 0;
// long ett_get_agg = 0;
// long sketch_query = 0;
// long sketch_time = 0;
// long refresh_time = 0;
// long parallel_isolated_check = 0;
// long tiers_grown = 0;
// long normal_refreshes = 0;


// template <typename SketchClass> requires(SketchColumnConcept<SketchClass, vec_t>)
// bool Batch<SketchClass>::is_connected(node_id_t a, node_id_t b) {
// 	return this->link_cut_tree.find_root(a) == this->link_cut_tree.find_root(b);
// }

template class BatchTiers<DefaultSketchColumn>;