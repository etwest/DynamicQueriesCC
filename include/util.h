#include "types.h"

static edge_id_t vertices_to_edge(node_id_t a, node_id_t b) {
   return a<b ? (((edge_id_t)a)<<32) + ((edge_id_t)b) : (((edge_id_t)b)<<32) + ((edge_id_t)a);
};
