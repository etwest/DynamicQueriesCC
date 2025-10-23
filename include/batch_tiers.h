#pragma once
#include "types.h"
#include <vector>
#include <atomic>
#include <parlay/sequence.h>
#include <parlay/primitives.h>
// #include <folly/AtomicHashArray.h>
#include <folly/concurrency/ConcurrentHashMap.h>

#include "euler_tour_tree.h"
// #include "link_cut_tree.h"
#include "lct_v2.h"
#include "union_find_local.h"
#include "sketchless_euler_tour_tree.h"
// #include "parlay_hash/unordered_set.h"

template <typename SketchClass = DefaultSketchColumn> requires(SketchColumnConcept<SketchClass, vec_t>)
class BatchTiers {
    private:
        size_t num_nodes;
        uint64_t seed;
        // size_t maximum_batch_size = 512;
        // size_t maximum_batch_size = 100;
        // size_t maximum_batch_size = 1 << 20;
        size_t maximum_batch_size = 1 << 20;
        // size_t maximum_batch_size = 1024;
        size_t granularity = 1 << 11;  // suggested number of tier-updates per thread 
        std::vector<EulerTourTree<SketchClass>> ett;  // one ETT for each tier
        LinkCutTreeMaxAgg<int8_t> link_cut_tree;
        SketchlessEulerTourTree<> query_ett;
        
        std::vector<GraphUpdate> transaction_log;

        // TODO - add the sketchless ETT for querying 
        // 

        // "root" nodes for each candidate component at each tier.
        union_find_local<int32_t> _component_reps_dsu;
                
        // static thread_local parlay::sequence<ColumnEntryDelta> _deltas_buffer;
        // static thread_local SketchClass _scratch_sketch;
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
        BatchTiers(node_id_t num_nodes, uint64_t seed);
        BatchTiers(node_id_t num_nodes, uint32_t num_tiers, int batch_size, size_t seed);
        ~BatchTiers();
        
        bool is_initialized(node_id_t u) {
            // no-op with vector implementation
            return ett[0].is_initialized(u);
        };
        
        void initialize_node(node_id_t u) {
            for (auto &tree: ett) {
                tree.initialize_node(u);
            }
            query_ett.initialize_node(u);
            link_cut_tree.initialize_node(u);
        }

        void uninitialize_node(node_id_t u) {
            for (auto &tree: ett) {
                tree.uninitialize_node(u);
            }
            query_ett.uninitialize_node(u);
            link_cut_tree.uninitialize_node(u);
        }
        
        void initialize_all_nodes() {
            // TODO - parallel_for?
            for (auto &tree: ett) {
                tree.initialize_all_nodes(num_nodes);
            }
            query_ett.initialize_all_nodes(num_nodes);
            link_cut_tree.initialize_all_nodes(num_nodes);
        }
        
        void flush_transaction_log() {
            transaction_log.clear();
        }

        const std::vector<GraphUpdate>& get_transaction_log() const {
            return transaction_log;
        }
        
        void process_all_updates() {
            if (update_buffer.size() > 0) {
                update_batch(update_buffer);
                update_buffer.clear();
            }
        }

        void update_batch(const parlay::sequence<GraphUpdate> &updates);
        
        bool is_tree_edge(node_id_t a, node_id_t b) {
            return query_ett.has_edge(a, b);
        }
        
        void update(const GraphUpdate &update) {
            // if (!is_initialized(update.edge.src) || !is_initialized(update.edge.dst)) {
            //     std::cout << "ruh oh" << std::endl;
            // }
            assert(this->is_initialized(update.edge.src));
            assert(this->is_initialized(update.edge.dst));
            // add to buffer:
            update_buffer.push_back(update);
            // bool is_tree_edge_deletion = (update.type == DELETE &&
            //                               is_tree_edge(update.edge.src, update.edge.dst));
            // if (update_buffer.size() >= maximum_batch_size || is_tree_edge_deletion) {
            if (update_buffer.size() >= maximum_batch_size) {
                // std::cout << "is_tree_edge_deletion: " << is_tree_edge_deletion << ", buffer size: " << update_buffer.size() << std::endl;
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

