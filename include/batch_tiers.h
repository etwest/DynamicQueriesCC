#pragma once
#include "types.h"
#include <vector>
#include <atomic>
#include <parlay/sequence.h>
#include <parlay/primitives.h>
// #include <folly/AtomicHashArray.h>
#include <folly/concurrency/ConcurrentHashMap.h>

#include "euler_tour_tree.h"
#include "link_cut_tree.h"
// #include "parlay_hash/unordered_set.h"

template <typename SketchClass = DefaultSketchColumn> requires(SketchColumnConcept<SketchClass, vec_t>)
class BatchTiers {
    private:
        size_t maximum_batch_size = 128;
        std::vector<EulerTourTree<SketchClass>> ett;  // one ETT for each tier
        LinkCutTree link_cut_tree;
        
        // matrix of [num_tiers x ( batch_size * 2 )]
        std::vector<parlay::sequence<SkipListNode<SketchClass>*>> _root_nodes;
        
        // jagged array: track isolated components/probably isolated components. 
        // why are we doing this instead of just using root_nodes?
        
        // a vector mapping each tier to the set of its components that need
        // to be checked for isolation
        std::vector<parlay::sequence<SkipListNode<SketchClass>*>> _updated_components;
        
        // tracks components that were already checked for isolation and had their
        // associated link/cut instructions logged.
        // parlay::sequence<SkipListNode<SketchClass>*> _current_isolated_components;
        folly::ConcurrentHashMap<node_id_t, uint32_t> _already_checked_components;
        
        // links to "broadcast" to all higher tiers
        parlay::sequence<Edge> _pending_links;
        // cuts to "broadcast" to all higher tiers, plus the index of the first tier
        // where the cut should be made
        parlay::sequence<std::pair<Edge, uint32_t>> _pending_cuts;
        
        
        
    public:
        BatchTiers(node_id_t num_nodes);
        ~BatchTiers();


        void update_batch(const parlay::sequence<GraphUpdate> &updates);
        // void update_batch(const parlay::slice<GraphUpdate> &updates);
        void is_connected(node_id_t a, node_id_t b);
        
        std::vector<std::set<node_id_t>> get_cc();
    private:
        // void refresh(const parlay::slice<GraphUpdate> &updates, bool did_cut);
        // TODO - determine what the helper functions are going to be
        SkipListNode<SketchClass>*& root_node(size_t tier, size_t update_idx, bool src_or_dst) {
            return _root_nodes[tier][update_idx * 2 + (src_or_dst ? 0 : 1)];
        }
        
    
    
};


template <typename SketchClass> requires(SketchColumnConcept<SketchClass, vec_t>)
void BatchTiers<SketchClass>::update_batch(const parlay::sequence<GraphUpdate> &updates) {
    
    // 1) STEP 1: Speculative non-tree edge update processing
    // in parallel, accros every tier and update,
    // update the ETT aggregates
    // then, reduce to find the maximum 
    size_t num_updates = updates.size();
    size_t num_tiers = ett.size();
    parlay::parallel_for(0, num_tiers*num_updates, [&](size_t i) {
        size_t tier = i / num_updates;
        size_t update_idx = i % num_updates;
        GraphUpdate update = updates[update_idx];
        // TODO - WE NEED TO NOT HANDLE CUTS IN HERE!! 
        // this breaks our assumption on ETTs not being able to structurally
        // change.
        if (update.type == DELETE && ett[tier].has_edge(update.edge.src, update.edge.dst)) {
            // ett[tier].cut(update.edge.src, update.edge.dst);
            // TODO - freak the hell out. This shouldn''t be happening here
        } else {
            // TODO - require specialized methods 
            SkipListNode<> *src_parent = ett[tier].update_sketch_atomic(update.edge.src, update_idx);
            SkipListNode<> *dst_parent = ett[tier].update_sketch_atomic(update.edge.dst, update_idx);
            
            root_node(tier, update_idx, true) = src_parent;
            root_node(tier, update_idx, false) = dst_parent;
        }
    });
    
    // we can use parlay::find, as long as we are using "tier-major" order
    auto isolation_tabulate = parlay::delayed_tabulate(
            (num_tiers - 1) * num_updates,
            [&](size_t i)
            {
                size_t tier = i / num_updates;
                size_t update_idx = i % num_updates;
                for (bool src_or_dst : {true, false})
                {
                    uint32_t tier_size = root_node(tier, update_idx, src_or_dst)->size;
                    uint32_t next_size = root_node(tier + 1, update_idx, src_or_dst)->size;
                    if (tier_size == next_size)
                    {
                        // This means that the component is isolated
                        // // TODO - decide what to do in this case
                        return true;
                    }
                }
                return false;
            });
    auto first_isolated_iter = parlay::find(isolation_tabulate, true);
    if (first_isolated_iter == isolation_tabulate.end()) {
        // no isolated components!
        return;
    }
    uint32_t first_isolated_idx = first_isolated_iter - isolation_tabulate.begin();
    uint32_t first_isolated_tier = first_isolated_idx / num_updates;

    // 3) proceed tier-serially: 
    // * at the first isolated tier, collect all components that are isolated.
    //  * each isolated component will give a new edge (a,b)
    //  * if a path exists already between a and b in the final tier, then cut the maximum weight
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
    for (uint32_t tier = first_isolated_tier; tier < ett.size()-1; tier++) {
        _updated_components[tier].clear();
    }
    for (uint32_t tier = first_isolated_tier; tier < ett.size()-1; tier++) {
        std::atomic_bool components_maximized(true);
        
        // TODO - can be parallel
        for (size_t update_idx = 0; update_idx < num_updates; update_idx++) {
            for (bool src_or_dst : {true, false}) {
                SkipListNode<SketchClass>* root = root_node(tier, update_idx, src_or_dst);
                SkipListNode<SketchClass>* actual_root = root->get_root();
                _updated_components[tier].push_back(actual_root);
            }
        }
        // now, _updated_components contains all components that need to be
        // checked for isolation
        parlay::parallel_for(0, _updated_components[tier].size(), [&](size_t i) {
            SkipListNode<SketchClass>* component_root = _updated_components[tier][i];
            // in case a component was previously merged already
            component_root = component_root->get_root();
            // we should skip this check
            if (_already_checked_components.find(component_root->node->vertex) != _already_checked_components.end()) {
                return;
            }
            // TODO - WHAT SHOULD THIS ACTUALLY BE
            SkipListNode<SketchClass> *next_tier_root =
                ett[tier + 1].get_root(component_root->node->vertex);
            SketchClass &ett_agg = component_root->sketch_agg;
            // TODO - do we want to sample before? idts. but we can at least
            // do the empty check with a special new primitive
            SketchSample query_result = ett_agg.sample();
            if (query_result.result != ZERO) {
                components_maximized.store(false, std::memory_order_relaxed);
            }

            if (component_root->size == next_tier_root->size) {
                if (query_result.result == GOOD) {
                    // this component is isolated, so we need to add it to the list
                    // _current_isolated_components.push_back(component_root);
                    // _current_isolated_components.insert(component_root->node->vertex);
                    _already_checked_components.insert(component_root->node->vertex, tier);

                    // .. and see if a path exists between the endpoints in the LCT
                    edge_id_t edge = query_result.idx;
                    node_id_t a = (node_id_t)edge;
                    node_id_t b = (node_id_t)(edge >> 32);

                    // check if a path exists between the endpoints
                    auto a_root = link_cut_tree.find_root(a);
                    auto b_root = link_cut_tree.find_root(b);

                    if (a_root == b_root) {
                        // a path exists, so we need to cut the maximum weight edge
                        // on the path
                        std::pair<edge_id_t, uint32_t> max_edge = link_cut_tree.path_aggregate(a, b);
                        node_id_t c = (node_id_t)max_edge.first;
                        node_id_t d = (node_id_t)(max_edge.first >> 32);
                        uint32_t first_appeared_tier = max_edge.second;
                        _pending_cuts.push_back({{c, d}, first_appeared_tier});
                    }
                    // regardless, we need to link on all higher tiers.
                    _pending_links.push_back({a, b});
                }
            }
        });
        
        // first: update the LCT with the pending links and cuts
        for (const Edge &link : _pending_links) {
            link_cut_tree.link(link.src, link.dst, tier + 1);
        }
        for (auto &cut : _pending_cuts) {
            link_cut_tree.cut(cut.first.src, cut.first.dst);           
        }
        
        // at this point, we know exactly what cuts and links we need to do at higher tiers.
        // for each tier, we'll perform the cuts and links, and then add any entries to _updated_components[tier] that
        // we need to.
        parlay::parallel_for(tier + 1, ett.size(), [&](size_t t) {
            for (auto &cut : _pending_cuts) {
                // do not perform cut if the edge has not yet appeared (duh?)
                if (cut.second < t) 
                    continue;
                // cut the edge in the current tier
                ett[t].cut(cut.first.src, cut.first.dst);
                // add the root node to the updated components list
                SkipListNode<SketchClass>* root = ett[t].get_root(cut.first.src);
                SkipListNode<SketchClass>* other_root = ett[t].get_root(cut.first.dst);
                _updated_components[t].push_back(root);
                _updated_components[t].push_back(other_root);
            }
            for (const Edge &link : _pending_links) {
                // link the two endpoints in the current tier
                ett[t].link(link.src, link.dst);
                // add the root node to the updated components list
                SkipListNode<SketchClass>* root = ett[t].get_root(link.src);
                _updated_components[t].push_back(root);
            }
        });
        
        // at this point, all links and cuts induced have been performed, and we have a log
        // of components that need to be checked for isolation in the next tier.
        _pending_links.clear();
        _pending_cuts.clear();
        _already_checked_components.clear();
        
        if (components_maximized.load(std::memory_order_relaxed)) {
            // if all components were maximized, we can skip the next tier
            // we know that at this point, there are no isolations at higher tiers.
            // because all potential isolated components must be a union of the modified components
            // found at this tier. so we can just return
            return;
        }

    }
};

