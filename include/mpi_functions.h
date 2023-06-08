#include <mpi.h>

static void bcast(void* message, int size, int root) {
    MPI_Bcast(message, size, MPI_BYTE, root, MPI_COMM_WORLD);
    std::cout << "BROADCAST DONE :)" << std::endl;
}