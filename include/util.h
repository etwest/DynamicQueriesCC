#include <chrono>
#include "types.h"

#define START(X) auto X = std::chrono::high_resolution_clock::now()
#define STOP(C, X) C += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - X).count()

//#define START(X) ;
//#define STOP(C, X) ;

#define VERTICES_TO_EDGE(A, B) A<B ? (((edge_id_t)A)<<32) + ((edge_id_t)B) : (((edge_id_t)B)<<32) + ((edge_id_t)A)