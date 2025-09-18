#include "../include/batch_tiers.h"
#include "util.h"
#include <random>
#include <atomic>
#include <tbb/tbb.h>

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

template <typename SketchClass> requires(SketchColumnConcept<SketchClass, vec_t>)
thread_local parlay::sequence<ColumnEntryDelta> BatchTiers<SketchClass>::_deltas_buffer = parlay::sequence<ColumnEntryDelta>();

template <typename SketchClass> requires(SketchColumnConcept<SketchClass, vec_t>)
BatchTiers<SketchClass>::BatchTiers(node_id_t num_nodes, uint64_t seed) : link_cut_tree(num_nodes), _component_reps_dsu(1), query_ett(num_nodes, 0, seed) {
	// Algorithm parameters
	uint32_t num_tiers = log2(num_nodes)/(log2(3)-1);
    // TODO - we can be a bit more ambitious?
    _component_reps_dsu = union_find_local<int32_t>(this->maximum_batch_size * 2);

	// Initialize all the ETTs
	std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0,MAX_INT);
    // int seed = dist(rng);
    std::cout << "SEED: " << seed << std::endl;
    rng.seed(seed);
	dist(rng); // To give 1:1 correspondence with MPI seeds
	for (uint32_t i = 0; i < num_tiers; i++) {
		int tier_seed = dist(rng);
		ett.emplace_back(num_nodes, i, tier_seed);
	}

	// Initialize the root nodes matrix
	_root_nodes.resize(num_tiers);
	for (auto& tier_roots : _root_nodes) {
		tier_roots.resize(maximum_batch_size * 2);
	}
    // and _updated_components
    _updated_components.resize(num_tiers);
}

template <typename SketchClass>
    requires(SketchColumnConcept<SketchClass, vec_t>)
BatchTiers<SketchClass>::BatchTiers(
    node_id_t num_nodes, uint32_t num_tiers, int batch_size, size_t seed) : 
link_cut_tree(num_nodes), _component_reps_dsu(1), query_ett(num_nodes, 0, seed) {
    // TODO - use the batch_size parameter?
    _component_reps_dsu = union_find_local<int32_t>(maximum_batch_size * 2);

    // Initialize all the ETTs
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0,MAX_INT);
    // int seed = dist(rng);
    std::cout << "SEED: " << seed << std::endl;
    rng.seed(seed);
    dist(rng); // To give 1:1 correspondence with MPI seeds
    for (uint32_t i = 0; i < num_tiers; i++) {
        int tier_seed = dist(rng);
        ett.emplace_back(num_nodes, i, tier_seed);
    }

    // Initialize the root nodes matrix
    _root_nodes.resize(num_tiers);
    for (auto& tier_roots : _root_nodes) {
        tier_roots.resize(maximum_batch_size * 2);
    }
    // and _updated_components
    _updated_components.resize(num_tiers);
}

template <typename SketchClass> requires(SketchColumnConcept<SketchClass, vec_t>)
BatchTiers<SketchClass>::~BatchTiers() {}


// TODO - check correctness on doing links/cuts out of order. lowkey it should be fine
// from a correctness pov
template <typename SketchClass> requires(SketchColumnConcept<SketchClass, vec_t>)
void BatchTiers<SketchClass>::update_batch(const parlay::sequence<GraphUpdate> &updates) {

    size_t num_updates = updates.size();
    size_t num_tiers = ett.size();
    assert(num_updates <= maximum_batch_size);
    _already_checked_components.clear();
    
    // treat all update endpoints as coming from independent components
    _component_reps_dsu.reset();

    // 0) Step 0: Process any necessary tree cut operations on every tier. 
    // we WONT immediately do the sketch updates in this case, and will rely on the next parallel branch for that
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, ett.size()),
        [&](const tbb::blocked_range<size_t>& r) {
            for (size_t i = r.begin(); i != r.end(); ++i) {
                for (const auto& update : updates) {
                    if (update.type == DELETE && ett[i].has_edge(update.edge.src, update.edge.dst)) {
                        ett[i].cut(update.edge.src, update.edge.dst);
                    }
                }
            }
        },
        tbb::static_partitioner{}
    );
    // note: can just put this in the above region or use pardo
    // and process on the LCT:
    for (const auto& update : updates) {
        if (update.type == DELETE && is_tree_edge(update.edge.src, update.edge.dst)) {
            link_cut_tree.cut(update.edge.src, update.edge.dst);
            query_ett.cut(update.edge.src, update.edge.dst);
            transaction_log.push_back(update);
        }
    }
    // 1) Step 1: Process all sketch aggs in true batch parallel.
    // _process_sketch_aggs_only(updates);
    _process_sketch_aggs_tier_sequential(updates);
    
    // 2) Step 2: Check for isolated components.
    uint32_t first_isolated_tier = _search_for_isolated_components(updates);
    if (first_isolated_tier == UINT32_MAX) {
        // no isolated components found, so we can return early
        return;
    }
    // the first isolated tier has had no link/cut modifications to it. so its roots array is a valid
    // check 
    // TODO - dont dynamically allocate this hash table
    // so we can use it to track components by their root node ptrs!
    // note that pairs have a lex sort defined!
    parlay::sequence<std::pair<size_t, int32_t>> component_roots = parlay::tabulate(num_updates * 2, [&](size_t i) {
            bool src_or_dst = static_cast<bool>(i % 2);
            size_t root_id = (size_t)static_cast<void *>(root_node(first_isolated_tier, i / 2, src_or_dst));
            return std::make_pair(root_id, static_cast<int32_t>(i));            
    });
    parlay::sort_inplace(component_roots);
    parlay::parallel_for(0, component_roots.size()-1, [&](size_t i) {
        // if the root is the same as the next one, we can union them
        if (component_roots[i].first == component_roots[i+1].first) {
            _component_reps_dsu.link(component_roots[i].second, component_roots[i+1].second);
        }
    });
    // for (size_t i =0; i < component_roots.size()-1; i++) {
    //     if (component_roots[i].first == component_roots[i+1].first) {
    //         // same root, so we can union them
    //         _component_reps_dsu.union_sets(component_roots[i].second, component_roots[i+1].second);
    //     }
    // }

    // 3) proceed tier-serially: 
    // * at the first isolated tier, collect all components that are isolated.
    //  * each isolated component will give a new edge (a,b)
    //  * if a path exists already between a and b in the final tier/LCT, then cut the maximum weight
    //    edge on the path, starting from the tier where it first appears (call it tier M) and going until the final one.
    //    
    //    (NOTE THAT tier M has to have a higher index than the first isolated tier. Because we know that
    //    the first isolated tier has a forest such that the endpoints of (a) and (b) were not connected.
    //    if there were a lower index tier, that wouldve violated the subset invariant.)
    //
    //  * if apath does not exist, then we link the two endpoints in all tiers ABOVE the first isolated tier.
    //    Note that this can cause NEW isolated components to appear in tiers above. 
    //
    //
    //  * once we do this for every isolated component at the first isolated tier, check the next tier
    //    to see if it has any isolated components. If it does, repeat (3) at the next tier.   
    //
    // 
    // SHORT CUTS: we can also tell if a component is maximized by checking for an empty sketch. This
    // means we can avoid doing further isolation checks. 
    // for (uint32_t)
    // TODO - is_empty check optimization
    // return;
    for (uint32_t tier = first_isolated_tier; tier < ett.size()-1; tier++) {
        _updated_components[tier].clear();
    }
    for (uint32_t tier = first_isolated_tier; tier < ett.size()-1; tier++) {
        bool components_maximized = _fix_isolations_at_tier(updates, tier);
        if (components_maximized) {
            // if all components were maximized, we can skip the next tier
            // we know that at this point, there are no isolations at higher tiers.
            // because all potential isolated components must be a union of the modified components
            // found at this tier. so we can just return
            return;
        }
    }
};

template <typename SketchClass> requires(SketchColumnConcept<SketchClass, vec_t>)
std::vector<std::set<node_id_t>> BatchTiers<SketchClass>::get_cc() {
    this->flush_buffer();
	std::vector<std::set<node_id_t>> cc;
	std::set<EulerTourNode<SketchClass>*> visited;
	int top = ett.size()-1;
	for (uint32_t i = 0; i < ett[top].ett_nodes.size(); i++) {
		if (visited.find(&ett[top].ett_nodes[i]) == visited.end()) {
			std::set<EulerTourNode<SketchClass>*> pointer_component = ett[top].ett_nodes[i].get_component();
			std::set<node_id_t> component;
			for (auto pointer : pointer_component) {
				component.insert(pointer->vertex);
				visited.insert(pointer);
			}
			cc.push_back(component);
		}
	}
	return cc;
}

template <typename SketchClass> requires(SketchColumnConcept<SketchClass, vec_t>)
bool BatchTiers<SketchClass>::is_connected(node_id_t a, node_id_t b) {
    this->flush_buffer();
    // TODO - use a sketchless ETT
	// return this->link_cut_tree.find_root(a) == this->link_cut_tree.find_root(b);
    return query_ett.is_connected(a, b); 
}

template <typename SketchClass> requires(SketchColumnConcept<SketchClass, vec_t>)
void BatchTiers<SketchClass>::_process_sketch_aggs_only(const parlay::sequence<GraphUpdate> &updates) {
    size_t num_updates = updates.size();
    size_t num_tiers = ett.size();
    assert(num_updates <= maximum_batch_size);
    // 1) STEP 1: Speculative non-tree edge update processing
    // (plus cleaning up and doing the updates for the tree edge deletions)
    // in parallel, accross every tier and update,
    // update the ETT aggregates
    // then, reduce to find the maximum 
    // TODO - make sure tree edge deletions arent being processed twice.
    // parlay::parallel_for(0, num_tiers*num_updates, [&](size_t i) {
    //     size_t tier = i / num_updates;
    //     size_t update_idx = i % num_updates;
    //     GraphUpdate update = updates[update_idx];
    //     vec_t edge_id = concat_pairing_fn(update.edge.src, update.edge.dst);
    //     SkipListNode<> *src_parent = ett[tier].update_sketch_atomic(update.edge.src, edge_id);
    //     SkipListNode<> *dst_parent = ett[tier].update_sketch_atomic(update.edge.dst, edge_id);

    //     root_node(tier, update_idx, true) = src_parent;
    //     root_node(tier, update_idx, false) = dst_parent;
    // }, granularity);

    // step 1 memory optimization:
    // enforce greater locality by first doing edges in
    // lower, higher sorted order (only do the srcs)
    // then in higher, lower (invert, then do dsts)
    auto src_sorted_update_idxs = parlay::tabulate(num_updates, [&](size_t i) {
        return i;
    });
    parlay::sort_inplace(src_sorted_update_idxs, [&](size_t i, size_t j) {
        return updates[i].edge.src < updates[j].edge.src;
    });
    auto dst_sorted_update_idxs = parlay::tabulate(num_updates, [&](size_t i) {
        return i;
    });
    parlay::sort_inplace(dst_sorted_update_idxs, [&](size_t i, size_t j) {
        return updates[i].edge.dst < updates[j].edge.dst;
    });

    // bool conservative=true;
    // do src updates:
    // parlay::blocked_for(0, num_updates * num_tiers, granularity, [&](size_t block_idx, size_t start, size_t end) {
        // for (size_t i = start; i < end; i++) {
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, num_updates * num_tiers, granularity),
        [&](const tbb::blocked_range<size_t> &r) {
            for (size_t i = r.begin(); i != r.end(); ++i) {
            size_t tier = i / num_updates;
            size_t update_idx = src_sorted_update_idxs[i % num_updates];
            GraphUpdate update = updates[update_idx];
            vec_t edge_id = concat_pairing_fn(update.edge.src, update.edge.dst);
            ColumnEntryDelta delta = ett[tier].generate_entry_delta(update.edge.src, edge_id);
            SkipListNode<SketchClass> *src_parent = ett[tier].update_sketch_atomic(update.edge.src, delta);
            root_node(tier, update_idx, true) = src_parent;
        }
    });
    // }, tbb::static_partitioner{});
    // }, conservative);
    // now dst updates:
    // parlay::blocked_for(0, num_updates * num_tiers, granularity, [&](size_t block_idx, size_t start, size_t end) {
        // for (size_t i = start; i < end; i++) {
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, num_updates * num_tiers, granularity),
        [&](const tbb::blocked_range<size_t> &r) {
            for (size_t i = r.begin(); i != r.end(); ++i) {
                size_t tier = i / num_updates;
                size_t update_idx = dst_sorted_update_idxs[i % num_updates];
                GraphUpdate update = updates[update_idx];
                vec_t edge_id = concat_pairing_fn(update.edge.src, update.edge.dst);
                ColumnEntryDelta delta = ett[tier].generate_entry_delta(update.edge.dst, edge_id);
                SkipListNode<SketchClass> *dst_parent = ett[tier].update_sketch_atomic(update.edge.dst, delta);
                root_node(tier, update_idx, false) = dst_parent;
                // }, conservative);}
            }
        });
    // tbb::static_partitioner{});
    // }, conservative);
}

template <typename SketchClass> requires(SketchColumnConcept<SketchClass, vec_t>)
void BatchTiers<SketchClass>::_process_sketch_aggs_tier_sequential(const parlay::sequence<GraphUpdate> &updates) {
    size_t num_updates = updates.size();
    size_t num_tiers = ett.size();
    assert(num_updates <= maximum_batch_size);
    auto src_sorted_update_idxs = parlay::tabulate(num_updates, [&](size_t i) {
        return i;
    });
    parlay::sort_inplace(src_sorted_update_idxs, [&](size_t i, size_t j) {
        return updates[i].edge.src < updates[j].edge.src;
    });
    auto dst_sorted_update_idxs = parlay::tabulate(num_updates, [&](size_t i) {
        return i;
    });
    parlay::sort_inplace(dst_sorted_update_idxs, [&](size_t i, size_t j) {
        return updates[i].edge.dst < updates[j].edge.dst;
    });

    // bool conservative=false;
    // bool conservative=true;
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, num_tiers, 1),
        [&](const tbb::blocked_range<size_t> &r) {
            for (size_t tier = r.begin(); tier != r.end(); ++tier) {
                for (size_t i = 0; i < num_updates; i++) {
                    size_t update_idx = src_sorted_update_idxs[i];
                    // size_t update_idx = i;
                    GraphUpdate update = updates[update_idx];
                    vec_t edge_id = concat_pairing_fn(update.edge.src, update.edge.dst);
                    // SkipListNode<SketchClass> *src_parent = ett[tier].update_sketch(update.edge.src, edge_id);
                    const ColumnEntryDelta delta = ett[tier].generate_entry_delta(update.edge.src, edge_id);
                    SkipListNode<SketchClass> *src_parent = ett[tier].update_sketch(update.edge.src, delta);
                    // SkipListNode<SketchClass> *src_parent = ett[tier].update_sketch_atomic(update.edge.src, delta);
                    
                    root_node(tier, update_idx, true) = src_parent;
                }
                for (size_t i = 0; i < num_updates; i++) {
                    root_node(tier, i, true)->process_updates();
                }
                for (size_t i = 0; i < num_updates; i++) {
                    size_t update_idx = dst_sorted_update_idxs[i];
                    // size_t update_idx = i;
                    GraphUpdate update = updates[update_idx];
                    vec_t edge_id = concat_pairing_fn(update.edge.src, update.edge.dst);
                    // SkipListNode<SketchClass> *dst_parent = ett[tier].update_sketch(update.edge.dst, edge_id);
                    const ColumnEntryDelta delta = ett[tier].generate_entry_delta(update.edge.dst, edge_id);
                    SkipListNode<SketchClass> *dst_parent = ett[tier].update_sketch(update.edge.dst, delta);
                    root_node(tier, update_idx, false) = dst_parent;
                }
                for (size_t i = 0; i < num_updates; i++) {
                    root_node(tier, i, false)->process_updates();
                }
            }
        },
        tbb::static_partitioner{}
    );
    // 0, conservative);
    // tbb::parallel_for(
    //     tbb::blocked_range<size_t>(0, num_tiers, 1),
    //     [&](const tbb::blocked_range<size_t> &r) {
    //         for (size_t tier = r.begin(); tier != r.end(); ++tier) {
    //             // for (size_t tier = 0; tier < num_tiers; tier++) {
    //             // source loop:
    //             size_t i = 0;
    //             while (i < num_updates) {
    //                 _deltas_buffer.clear();
    //                 size_t j = i;
    //                 while (j < num_updates && updates[src_sorted_update_idxs[j]].edge.src == updates[src_sorted_update_idxs[i]].edge.src) {
    //                     GraphUpdate update = updates[src_sorted_update_idxs[j]];
    //                     vec_t edge_id = concat_pairing_fn(
    //                         update.edge.src,
    //                         update.edge.dst);
    //                     auto delta = ett[tier].generate_entry_delta(
    //                         update.edge.src,
    //                         edge_id);
    //                     _deltas_buffer.push_back(delta);

    //                     j++;
    //                 }
    //                 SkipListNode<SketchClass> *src_parent = this->ett[tier].update_sketch(
    //                     updates[src_sorted_update_idxs[i]].edge.src,
    //                     _deltas_buffer.head(_deltas_buffer.size()));
    //                 for (size_t k = i; k < j; k++) {
    //                     size_t update_idx = src_sorted_update_idxs[k];
    //                     root_node(tier, update_idx, true) = src_parent;
    //                 }
    //                 i = j;
    //             }
    //             // dest loop:
    //             i = 0;
    //             while (i < num_updates) {
    //                 _deltas_buffer.clear();
    //                 size_t j = i;
    //                 while (j < num_updates && updates[dst_sorted_update_idxs[j]].edge.dst == updates[dst_sorted_update_idxs[i]].edge.dst) {
    //                     GraphUpdate update = updates[dst_sorted_update_idxs[j]];
    //                     vec_t edge_id = concat_pairing_fn(
    //                         update.edge.src,
    //                         update.edge.dst);
    //                     auto delta = ett[tier].generate_entry_delta(
    //                         update.edge.dst,
    //                         edge_id);
    //                     _deltas_buffer.push_back(delta);
    //                     j++;
    //                 }
    //                 SkipListNode<SketchClass> *dst_parent = this->ett[tier].update_sketch(
    //                     updates[dst_sorted_update_idxs[i]].edge.dst,
    //                     _deltas_buffer.head(_deltas_buffer.size()));
    //                 for (size_t k = i; k < j; k++) {
    //                     size_t update_idx = dst_sorted_update_idxs[k];
    //                     root_node(tier, update_idx, false) = dst_parent;
    //                 }
    //                 i = j;
    //             }
    //             // parlay::parallel_for(0, num_updates, [&](size_t k) {
    //             //     root_node(tier, k, true)->process_updates();
    //             //     root_node(tier, k, false)->process_updates();
    //             // });
    //         }
    //     }
    //     // tbb::static_partitioner{}
    // );
}

template<typename SketchClass> requires(SketchColumnConcept<SketchClass, vec_t>)
uint32_t BatchTiers<SketchClass>::_search_for_isolated_components(const parlay::sequence<GraphUpdate> &updates) {
    size_t num_updates = updates.size();
    size_t num_tiers = ett.size();
    assert(num_updates <= maximum_batch_size);
    // we can use parlay::find, as long as we are using "tier-major" order
    auto isolation_tabulate = parlay::delayed_tabulate(
        (num_tiers - 1) * num_updates,
        [&](size_t i) {
            size_t tier = i / num_updates;
            size_t update_idx = i % num_updates;
            for (bool src_or_dst : {true, false}) {
                SkipListNode<SketchClass> *root = root_node(tier, update_idx, src_or_dst);
                SkipListNode<SketchClass> *next_root = root_node(tier + 1, update_idx, src_or_dst);
                uint32_t tier_size = root->size;
                uint32_t next_size = next_root->size;
                if (tier_size == next_size) {
                    // This means that the component is isolated
                    if (root->sketch_agg.sample().result == GOOD) {
                        // this means that the component is isolated
                        // std::cout << "isolation found at tier " << tier << " for update idx " << update_idx << std::endl;
                        return true;
                    }
                }
            }
            return false;
        });
    auto first_isolated_iter = parlay::find(isolation_tabulate, true);
    if (first_isolated_iter == isolation_tabulate.end()) {
        // no isolated components!
        return UINT32_MAX;
    }
    uint32_t first_isolated_idx = first_isolated_iter - isolation_tabulate.begin();
    // note - i dont think we care about the isolation idx
    uint32_t first_isolated_tier = first_isolated_idx / num_updates;
    return first_isolated_tier;
}

template<typename SketchClass> requires(SketchColumnConcept<SketchClass, vec_t>)
bool BatchTiers<SketchClass>::_fix_isolations_at_tier(const parlay::sequence<GraphUpdate> &updates, uint32_t tier) {
    size_t num_updates = updates.size();
    // size_t num_tiers = ett.size();

    // needs to be atomically updated.
    bool components_maximized = true;

    // TODO - can be parallel
    // for (size_t update_idx = 0; update_idx < num_updates; update_idx++) {
    //     // for (bool src_or_dst : {true, false}) {
    //     //     SkipListNode<SketchClass>* root = root_node(tier, update_idx, src_or_dst);
    //     //     SkipListNode<SketchClass>* actual_root = root->get_root();
    //     //     _updated_components[tier].push_back(actual_root);
    //     // }
    //     _updated_components[tier].push_back(updates[update_idx].edge.src);
    //     _updated_components[tier].push_back(updates[update_idx].edge.dst);
    // }
    // for each update, we only need to grab ROOTS
    // for (size_t i=0; i < num_updates * 2; i++) {
    // parlay::parallel_for(0, num_updates * 2, [&](size_t i) {
    for (size_t i = 0; i < num_updates * 2; i++) {
        // only if you are STILL a root.
        // AND your sketch is non-empty
        if (!_component_reps_dsu.is_root(i)) {
            // return;
            continue;
        }
        bool src_or_dst = static_cast<bool>(i % 2);
        size_t update_idx = i / 2;
        _updated_components[tier].push_back(
            src_or_dst ? updates[update_idx].edge.src : updates[update_idx].edge.dst);
    }
    // });
    // now, _updated_components contains all components that need to be
    // including ones that may have been inherited from doing links/cuts below.
    for (size_t i = 0; i < _updated_components[tier].size(); i++) {
        node_id_t vertex_in_component = _updated_components[tier][i];
        // TODO - we can do some work to avoid checking the same component (maybe?)
        // in case a component was previously merged already
        SkipListNode<SketchClass> *component_root = ett[tier].get_root(vertex_in_component);
        SkipListNode<SketchClass> *next_tier_root = ett[tier + 1].get_root(vertex_in_component);
        
        // TODO - this is no longer necessary. because we are using the DSU to keep the smallest
        // possible set of _updated_components settings
        // actually, we'll keep it for now anyway.
        // this is because the current DSU filter is just being used as a simple filter.
        // since we arent doing any changes to it past the first isolated tier.
        if (_already_checked_components.find((size_t)(component_root)) != _already_checked_components.end()) {
            // std::cout << "yerr" << std::endl;
            // return;
            continue;
        }
        _already_checked_components.insert_or_assign((size_t)component_root, tier);
        SketchClass &ett_agg = component_root->sketch_agg;
        // TODO - do we want to sample before? idts. but we can at least
        // do the empty check with a special new primitive
        SketchSample query_result = ett_agg.sample();
        if (query_result.result != ZERO) {
            if (components_maximized) {
                // bool f = false;
                // bool t = true;
                __sync_bool_compare_and_swap((bool *)&components_maximized, true, false);
            }
        }

        if (component_root->size == next_tier_root->size) {
            if (query_result.result == GOOD) {
                // this component is isolated, so we need to add it to the list
                // _current_isolated_components.push_back(component_root);
                // _current_isolated_components.insert(component_root->node->vertex);

                // .. and see if a path exists between the endpoints in the LCT
                edge_id_t edge = query_result.idx;
                node_id_t a = (node_id_t)edge;
                node_id_t b = (node_id_t)(edge >> 32);

                // check if a path exists between the endpoints
                auto a_root = link_cut_tree.find_root(a);
                auto b_root = link_cut_tree.find_root(b);
                // TODO - ETT

                // if it does, then we either need to cut it, or ignore this update

                if (a_root == b_root) {
                    // a path exists, so we need to cut the maximum weight edge
                    // on the path
                    // THIS REALLY CANT BE PARALLELIZED atm
                    std::pair<edge_id_t, uint32_t> max_edge = link_cut_tree.path_aggregate(a, b);
                    node_id_t c = (node_id_t)max_edge.first;
                    node_id_t d = (node_id_t)(max_edge.first >> 32);
                    uint32_t first_appeared_tier = max_edge.second;
                    // if the first appeared tier is equal to tier+1, then we should check if this
                    // was a link we had just discovered. If so, we neither cut it, not include this link.
                    if (first_appeared_tier == tier + 1) {
                        // YOU KNOW that these couldnt have been connected in the tier above
                        // because otherwise the components coulld not have been the same size
                        // (which is necessary for isolation condition)
                        //
                        // so: DO NOTHING
                    } else {
                        // likewise, if it's a higher tier, definitely perform the cut
                        _pending_cuts.push_back({{c, d}, first_appeared_tier});
                        link_cut_tree.cut(c, d);
                        query_ett.cut(c, d);
                        transaction_log.push_back({{c, d}, DELETE});

                        // and push the link we just found
                        _pending_links.push_back({a, b});
                        link_cut_tree.link(a, b, tier + 1);
                        query_ett.link(a, b);
                        transaction_log.push_back({{a, b}, INSERT});
                        // and update the dsu
                    }
                } else {
                    // if there was no competing link between the endpoints in the LCT,
                    // then we just link them.
                    _pending_links.push_back({a, b});
                    link_cut_tree.link(a, b, tier + 1);
                    query_ett.link(a,b);
                    transaction_log.push_back({{a, b}, INSERT});
                }
            }
        }
    }

    // at this point, we know exactly what cuts and links we need to do at higher tiers.
    // for each tier, we'll perform the cuts and links, and then add any entries to _updated_components[tier] that
    // we need to.
    // parlay::parallel_for(tier + 1, ett.size(), [&](size_t t) {
    tbb::parallel_for(
        tbb::blocked_range<size_t>(tier + 1, ett.size(), 1),
        [&](const tbb::blocked_range<size_t> &r) {
            for (size_t t = r.begin(); t != r.end(); ++t) {
                // for (size_t t = tier + 1; t < ett.size(); t
                for (auto &cut : _pending_cuts) {
                    // do not perform cut if the edge has not yet appeared (duh?)
                    if (cut.second < t)
                        continue;
                    // cut the edge in the current tier
                    ett[t].cut(cut.first.src, cut.first.dst);
                }
                for (const Edge &link : _pending_links) {
                    ett[t].link(link.src, link.dst);
                }
            }
        },
        tbb::static_partitioner{});
    // });

    // at this point, all links and cuts induced have been performed, and we have a log
    // of components that need to be checked for isolation in the next tier.
    _pending_links.clear();
    _pending_cuts.clear();
    _already_checked_components.clear();

    return components_maximized;

}

template class BatchTiers<DefaultSketchColumn>; 