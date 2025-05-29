#include "mpi_nodes.h"
#include <dycon/localTree/SCCWN.hpp>

class HybridConnectivityManager {
    private:
        node_id_t num_nodes;
        InputNode sketching_algo;
        SCCWN<> cf_algo;
        // TODO - NOT the right way to do this at all.
        // LOTS of wasted space and potentially time on stuff
        absl::flat_hash_map<node_id_t, absl::flat_hash_set<node_id_t>> cf_edges;
        
        // tracks which of our CF edges are from the sketching algo
        absl::flat_hash_set<edge_id_t> edges_from_sketch;
        void flush_transaction_log() {
            for (auto &update: sketching_algo.get_transaction_log()) {
                if (update.type == DELETE) {
                    cf_edges[update.edge.src].erase(update.edge.dst);
                    cf_edges[update.edge.dst].erase(update.edge.src);
                    cf_algo.remove(update.edge.src, update.edge.dst);
                }
                else {
                    cf_edges[update.edge.src].insert(update.edge.dst);
                    cf_edges[update.edge.dst].insert(update.edge.src);
                    cf_algo.insert(update.edge.src, update.edge.dst);
                }
            }
            sketching_algo.flush_transaction_log();
        }

    public:
        HybridConnectivityManager(node_id_t num_nodes, uint32_t num_tiers, int batch_size, int seed)
            : num_nodes(num_nodes), sketching_algo(num_nodes, num_tiers, batch_size, seed), cf_algo(num_nodes) {}

        ~HybridConnectivityManager() {}
        
        void flush_edges_to_sketch(node_id_t vertex_to_flush) {
            // flush all "dense" edges from the CF to the sketching algo

            // TODO - determine what is a dense edge (i.e., figure ho)
            
            // TODO - we want to make the determination that we have enough
            // dense edges to justify a flush in constant time
        }

        void update(GraphUpdate update) {
            // external garauntee: well-formed stream. a remove is only called if the edge exists
            if (update.type == INSERT) {
                cf_edges[update.edge.src].insert(update.edge.dst);                
                cf_edges[update.edge.dst].insert(update.edge.src);
                cf_algo.insert(update.edge.src, update.edge.dst);
                // TODO - EVENTUALLY implement a check to see if a flush is needed
                if (0) {
                    flush_edges_to_sketch(update.edge.src);
                    flush_edges_to_sketch(update.edge.dst);
                }
            }
            else {
                // TODO - eventually do more precise casework
                // 1) edge exists in the CF:
                //      * a) edge originally comes from the sketch forest: update the sketch algo; apply transaction log
                //      * b) edge originally comes from the CF: remove it from the CF and you're done.
                if (cf_edges[update.edge.src].find(update.edge.dst) != cf_edges[update.edge.src].end()) {
                    edge_id_t edge_id = VERTICES_TO_EDGE(update.edge.src, update.edge.dst);
                    if (edges_from_sketch.find(edge_id) != edges_from_sketch.end()) {
                        // case a)
                        sketching_algo.update(update);
                        flush_transaction_log();
                    }
                    else {
                        //case b)
                        cf_edges[update.edge.src].erase(update.edge.dst);
                        cf_edges[update.edge.dst].erase(update.edge.src);
                        cf_algo.remove(update.edge.src, update.edge.dst);
                    }
                }
                // 2) edge does not exist in the CF:
                //  * it must be in the sketch algo, so update the sketch algo and apply transaction log.
                else {
                    sketching_algo.update(update);
                    flush_transaction_log();
                }
                // TODO - eventually implement a check to see if we need to remove
                // one of the vertices from the sketch algo and dump the edges out.
            }
        }

        bool connectivity_query(node_id_t a, node_id_t b) {
            return cf_algo.is_connected(a, b);
        }

};