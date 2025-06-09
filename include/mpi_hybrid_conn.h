#include "mpi_nodes.h"
#include "graph_tiers.h"
#include <dycon/localTree/SCCWN.hpp>
#include "recovery.h"

class HybridConnectivityManager {
    // TODO 
    public:
        // TODO - make this not public
        InputNode sketching_algo;
    private:
        
        size_t seed;
        node_id_t num_nodes;
        // GraphTiers<DefaultSketchColumn> sketching_algo;
        SCCWN<> cf_algo;
        // TODO - move semantics for sparserecovery?
        absl::flat_hash_map<node_id_t, SparseRecovery*> recovery_sketches;
        
        
        // tracks which of our CF edges are from the sketching algo
        absl::flat_hash_set<edge_id_t> edges_from_sketch;
        
        // tracks how many dense edges are still in the CF
        // generate plot with varying batch size
        // keeping a global buffer is likely sufficient
        // doing vertex-level might make checkpointing harder - think about this
        std::vector<uint16_t> num_pending_dense_edges;
        
        // buffer for when we need to collect all neighbors
        std::vector<node_id_t> _neighbors_buffer;

        // TODO - this might be replaced by something internal to modified-cupcake
        // can also just be a vector probably
        absl::flat_hash_set<node_id_t> _is_vertex_sketched;

        static constexpr size_t MOVE_TO_SKETCH = 500;
        
        size_t count_explicit_neighbors(node_id_t vertex) {
            return cf_algo.leaves[vertex]->getSize();
        }
        
        bool is_forest_edge_from_sketch(Edge edge) {
            // TODO - watch out for performance penalty of this.
            // might be a reason to use an alternate scheme
            return edges_from_sketch.find(VERTICES_TO_EDGE(edge.src, edge.dst)) != edges_from_sketch.end();
        }
        
        bool is_edge_in_cf(Edge edge) {
            // TODO - watch out for performance penalty of this.
            // might be a reason to use an alternate scheme
            return cf_algo.leaves[edge.src]->getEdgeLevel(edge.dst) != MAX_LEVEL + 2; 
        }

        bool is_vertex_sketched(node_id_t vertex) {
            return _is_vertex_sketched.find(vertex) != _is_vertex_sketched.end();
        }
        
        void initialize_vertex_sketch(node_id_t vertex) {
            // TODO - is basically a no-op from the perspective of the sketching algo
            _is_vertex_sketched.insert(vertex);
            recovery_sketches.emplace(vertex, new SparseRecovery(num_nodes, MOVE_TO_SKETCH, 2.0, seed ));
            
            // update your neighbors' dense edge counts
            for (size_t level=0; level < MAX_LEVEL; level++) {
                auto edge_set = localTree::getEdgeSet(cf_algo.leaves[vertex], level);
                if (edge_set) {
                    for (node_id_t neighbor: *edge_set) {
                        if (is_vertex_sketched(neighbor)) {
                            num_pending_dense_edges[neighbor]++;
                        }
                    }
                }
            }
        }

        void uninitialize_vertex_sketch(node_id_t vertex) {
            // TODO - is basically a no-op from the perspective of the sketching algo
            _is_vertex_sketched.erase(vertex);
            delete[] recovery_sketches[vertex];
            recovery_sketches.erase(vertex);
            
            //update your neighbors' dense edge counts
            for (size_t level=0; level < MAX_LEVEL; level++) {
                auto edge_set = localTree::getEdgeSet(cf_algo.leaves[vertex], level);
                if (edge_set) {
                    for (node_id_t neighbor: *edge_set) {
                        if (is_vertex_sketched(neighbor)) {
                            num_pending_dense_edges[neighbor]--;
                        }
                    }
                }
            }
        }
        
        void flush_transaction_log() {
            for (auto &update: sketching_algo.get_transaction_log()) {
                // TODO - defer the calls to cf_algo in order to do a bulk 
                // insertion!
                if (update.type == DELETE) {
                    // cf_edges[update.edge.src].erase(update.edge.dst);
                    // cf_edges[update.edge.dst].erase(update.edge.src);
                    cf_algo.remove(update.edge.src, update.edge.dst);
                }
                else {
                    // cf_edges[update.edge.src].insert(update.edge.dst);
                    // cf_edges[update.edge.dst].insert(update.edge.src);
                    cf_algo.insert(update.edge.src, update.edge.dst);
                }
            }
            sketching_algo.flush_transaction_log();
        }

    public:
        HybridConnectivityManager(node_id_t num_nodes, uint32_t num_tiers, int batch_size, size_t seed)
            : num_nodes(num_nodes), sketching_algo(num_nodes, num_tiers, batch_size, seed), cf_algo(num_nodes), seed(seed) {
                num_pending_dense_edges.resize(num_nodes, 0);
            }

        ~HybridConnectivityManager() {}
        void flush_edges_to_sketch(node_id_t vertex_to_flush) {
            // 1) find all edges incident to vertex_to_flush AND to a dense edge
            _neighbors_buffer.clear();
            for (size_t level=0; level < MAX_LEVEL; level++) {
                auto edge_set = localTree::getEdgeSet(cf_algo.leaves[vertex_to_flush], level);
                if (edge_set) {
                    for (node_id_t neighbor: *edge_set) {
                        // TODO - double check if this is the right way to do this
                        if (is_vertex_sketched(neighbor) && !is_forest_edge_from_sketch(Edge{vertex_to_flush, neighbor}))
                        {
                            // if the edge is not from the sketching algo, and it's connected to a dense vertex
                            // add it to the buffer and 
                            // and increment the pending dense edge count
                            _neighbors_buffer.push_back(neighbor);
                        }
                    }
                }
            }
            // reason for separate loops: see if improvements can be had from figuring out
            // a bulk insertion strategy
            
            // 2) increment their pending_dense_edge counts (but don't flush them yourself)
            // (since this vertex is about to densify)
            for (node_id_t neighbor: _neighbors_buffer) {
                num_pending_dense_edges[neighbor]++;
            }
            // remove edges from the cluster forest
            for (node_id_t neighbor: _neighbors_buffer) {
                cf_algo.remove(vertex_to_flush, neighbor);
            }
            
            // 3) insert them into the sketching algo
            for (node_id_t neighbor: _neighbors_buffer) {
                if (neighbor != vertex_to_flush) {
                    sketching_algo.update(GraphUpdate{Edge{vertex_to_flush, neighbor}, INSERT});
                    // TODO - ensure this is initialized
                    recovery_sketches[vertex_to_flush]->update(VERTICES_TO_EDGE(vertex_to_flush, neighbor));
                }
            }
            // apply the transaction log
            flush_transaction_log();
                        

            
        }
        
        bool check_and_perform_recovery(node_id_t vertex) {
            /*
                Assumes the vertex is sketched
                Checks if the recovery sketch is sufficiently sparse
                If so, performs a recovery attempt
            */
            // or use the explicit degree because of well-formed stream assumption 
            likely_if (!recovery_sketches[vertex]->worth_recovery_attempt()) {
                return false;
            }
            auto recovery_attempt = recovery_sketches[vertex]->recover();
            unlikely_if (recovery_attempt.result = FAILURE) {
                // TODO - handle failure case
                return false;
            }
            // then remove the edge from neighbors' recovery structures
            for (vec_t &vec: recovery_attempt.recovered_indices) {
                Edge edge = inv_concat_pairing_fn(vec);
                node_id_t other_vertex = edge.src == vertex ? edge.dst : edge.src;
                recovery_sketches[other_vertex]->update(vec);
            }
            // and flush the edges out of the sketching algo
            for (vec_t &vec: recovery_attempt.recovered_indices) {
                Edge edge = inv_concat_pairing_fn(vec);
                sketching_algo.update(GraphUpdate{edge, DELETE});
            }
            // and apply the transaction log
            flush_transaction_log();
            // and add the edges back to the cluster forest
            // NOTE - WE KNOW THAT none of the edges are already in the cluster forest
            // this is because we applied the transaction log, so any edges in the forest that
            // came for a sketch forest were removed.
            // TODO - it might be worth thinking about this and optimizing
            //     i.e. if we just apply the transaction log, we might delete an edge from the cf,
            //     and then put it right back here later.
            for (vec_t &vec: recovery_attempt.recovered_indices) {
                Edge edge = inv_concat_pairing_fn(vec);
                cf_algo.insert(edge.src, edge.dst);
            }
            // now we can clear the recovery data structure
            uninitialize_vertex_sketch(vertex);
            
        }

        void update(GraphUpdate update) {
            // external garauntee: well-formed stream. a remove is only called if the edge exists
            // would be nice to get rid of assumption
            if (update.type == INSERT) {
                cf_algo.insert(update.edge.src, update.edge.dst);
                
                // check to see if we densified the vertices enough to initialize their sketches
                unlikely_if (count_explicit_neighbors(update.edge.src) >= MOVE_TO_SKETCH) {
                    // these functions should be no-ops on dense edges
                    initialize_vertex_sketch(update.edge.src);
                }
                unlikely_if (count_explicit_neighbors(update.edge.dst) >= MOVE_TO_SKETCH) {
                    initialize_vertex_sketch(update.edge.dst);
                }
                
                // logic for updating pending dense edge counts + potentially flushing out
                // dense edges to the sketching structure
                for (std::pair<node_id_t, node_id_t> e: {
                    std::make_pair(update.edge.src, update.edge.dst),
                    std::make_pair(update.edge.dst, update.edge.src)
                }) {
                    auto v1 = e.first;
                    auto v2 = e.second;
                    if (is_vertex_sketched(v2)) {
                        if (++num_pending_dense_edges[v1] >= MOVE_TO_SKETCH) {
                            // TODO - ensure this is a no-op if already initialized
                            initialize_vertex_sketch(v1);
                            // flush the edges to the sketching algo
                            num_pending_dense_edges[v1] = 0;
                            flush_edges_to_sketch(v1);
                        }
                    }
                }
            }
            else if (update.type == DELETE) {

                // TODO - eventually do more precise casework
                // if edge exists in the CF (1):
                //      * a) edge originally comes from the sketch forest: update the sketch algo; apply transaction log
                //      * b) edge originally comes from the CF: remove it from the CF and you're done.
                
                // if not in cluster forest (2):
                // TODO - this logic should check the cf for which edges exist in it
                // if (cf_edges[update.edge.src].find(update.edge.dst) != cf_edges[update.edge.src].end()) {
                // if (cf_algo.has_edge(update.edge.src, update.edge.dst)) {
                if (this->is_edge_in_cf(update.edge)) {
                    edge_id_t edge_id = VERTICES_TO_EDGE(update.edge.src, update.edge.dst);
                    // if edge comes from sketching algo:
                    if (edges_from_sketch.find(edge_id) != edges_from_sketch.end()) {
                        // case a)
                        sketching_algo.update(update);
                        recovery_sketches[update.edge.src]->update(VERTICES_TO_EDGE(update.edge.src, update.edge.dst));
                        recovery_sketches[update.edge.dst]->update(VERTICES_TO_EDGE(update.edge.src, update.edge.dst));
                        flush_transaction_log();
                        check_and_perform_recovery(update.edge.src);
                        check_and_perform_recovery(update.edge.dst);
                        // can we do defered work: yes
                        // do we have to: ??? figure out
                    }
                    else {
                        //case b)
                        cf_algo.remove(update.edge.src, update.edge.dst);

                        // TODO - same logic is needed as above to DECREMENT pending dense edges
                        // in the sparse part, if this were the case.

                        if (is_vertex_sketched(update.edge.dst))
                        {
                            num_pending_dense_edges[update.edge.src]--;
                        }

                        if (is_vertex_sketched(update.edge.src))
                        {
                            num_pending_dense_edges[update.edge.dst]--;
                        }
                    }
                }
                // 2) edge does not exist in the CF:
                //  * it must be in the sketch algo, so update the sketch algo and apply transaction log.
                else {
                    // THIS IS THE OBVIOUS BUFFERING CASE FOR DELETIONS
                    sketching_algo.update(update);
                    recovery_sketches[update.edge.src]->update(VERTICES_TO_EDGE(update.edge.src, update.edge.dst));
                    recovery_sketches[update.edge.dst]->update(VERTICES_TO_EDGE(update.edge.src, update.edge.dst));
                    // TODO - verify that we don't need to flush transaction log
                    // flush_transaction_log();
                    check_and_perform_recovery(update.edge.src);
                    check_and_perform_recovery(update.edge.dst);
                }
                // TODO - eventually implement a check to see if we need to remove
                // one of the vertices from the sketch algo and dump the edges out.
            }
        }

        bool connectivity_query(node_id_t a, node_id_t b) {
            return cf_algo.is_connected(a, b);
        }
        
        std::vector<std::set<node_id_t>> cc_query() {
            // TODO - this aint great.
            std::vector<std::set<node_id_t>> ret;
            std::unordered_map<node_id_t, std::set<node_id_t>> component_map;
            for (node_id_t i=0; i < num_nodes; i++) {
                auto root = localTree::getRoot(cf_algo.leaves[i]);
                node_id_t root_id = root->get_id();
                if (root_id != ((node_id_t)-1) && component_map.find(root_id) == component_map.end()) {
                    component_map[root_id] = std::set<node_id_t>();
                    component_map[root_id].insert(root_id);
                }
                component_map[root_id].insert(i);
            }
            for (auto &pair: component_map) {
                ret.push_back(pair.second);
            }
        }

};