#include <chrono>

#define START(X) auto X = std::chrono::high_resolution_clock::now()
#define STOP(C, X) C += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - X).count()


//#define START(X) ;
//#define STOP(C, X) ;