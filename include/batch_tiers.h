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
#include "union_find.h"
// #include "parlay_hash/unordered_set.h"

template <typename SketchClass = DefaultSketchColumn> requires(SketchColumnConcept<SketchClass, vec_t>)
class BatchTiers {
    private:
        // size_t maximum_batch_size = 512;
        // size_t maximum_batch_size = 100;
        size_t maximum_batch_size = 1 << 20;
        // size_t maximum_batch_size = 1024;
        size_t granularity = 1 << 17;  // suggested number of tier-updates per thread 
        std::vector<EulerTourTree<SketchClass>> ett;  // one ETT for each tier
        LinkCutTree link_cut_tree;
        // TODO - add the sketchless ETT for querying 
        // 

        // "root" nodes for each candidate component at each tier.
        union_find<int32_t> _component_reps_dsu;
                
        // matrix of [num_tiers x ( batch_size * 2 )]
        std::vector<parlay::sequence<SkipListNode<SketchClass>*>> _root_nodes;
        
        // jagged array: track isolated components/probably isolated components. 
        // why are we doing this instead of just using root_nodes?
        
        // a vector mapping each tier to the set of its components that need
        // to be checked for isolation
        // parlay::sequence<parlay::sequence<SkipListNode<SketchClass>*>> _updated_components;
        // TODO - see if we can get rid of redundant checks
        // and only do one PER component. ie if some components share the same
        // root, we need not check them.
        // parlay::sequence<parlay::sequence<SkipListNode<SketchClass>*>> _updated_components;
        parlay::sequence<parlay::sequence<node_id_t>> _updated_components;
        
        // tracks components that were already checked for isolation and had their
        // associated link/cut instructions logged.
        // parlay::sequence<SkipListNode<SketchClass>*> _current_isolated_components;
        
        // key: a root node ptr (to identify same component at current tier)
        folly::ConcurrentHashMap<size_t, node_id_t> _already_checked_components;
        
        // links to "broadcast" to all higher tiers
        parlay::sequence<Edge> _pending_links;
        // cuts to "broadcast" to all higher tiers, plus the index of the first tier
        // where the cut should be made
        parlay::sequence<std::pair<Edge, uint32_t>> _pending_cuts;
        
        parlay::sequence<GraphUpdate> update_buffer;
        
        
        
    public:
        BatchTiers(node_id_t num_nodes);
        ~BatchTiers();


        void update_batch(const parlay::sequence<GraphUpdate> &updates);
        
        void update(const GraphUpdate &update) {
            // add to buffer:
            update_buffer.push_back(update);
            bool is_tree_edge_deletion = (update.type == DELETE &&
                                          link_cut_tree.has_edge(update.edge.src, update.edge.dst));
            if (update_buffer.size() >= maximum_batch_size || is_tree_edge_deletion) {
                // process the batch
                update_batch(update_buffer);
                // clear the buffer
                update_buffer.clear();
            }
        }
        
        void flush_buffer() {
            if (update_buffer.size() > 0) {
                update_batch(update_buffer);
                update_buffer.clear();
            }
        }

        bool is_connected(node_id_t a, node_id_t b);

        // query for the connected components of the graph
        std::vector<std::set<node_id_t>> get_cc();
    private:
        SkipListNode<SketchClass>*& root_node(size_t tier, size_t update_idx, bool src_or_dst) {
            return _root_nodes[tier][update_idx * 2 + (src_or_dst ? 0 : 1)];
        };
        void _process_sketch_aggs_only(const parlay::sequence<GraphUpdate> &updates);
        
        
        // same thing but seperates by tiers. this avoids the needs for atomics.
        void _process_sketch_aggs_tier_sequential(const parlay::sequence<GraphUpdate> &updates);

        uint32_t _search_for_isolated_components(const parlay::sequence<GraphUpdate> &updates);
        
        bool _fix_isolations_at_tier(const parlay::sequence<GraphUpdate> &updates, uint32_t tier_idx);
};

