#include <mpi.h>

static void bcast(void* message, int size, int root) {
    MPI_Bcast(message, size, MPI_BYTE, root, MPI_COMM_WORLD);
}

static void gather(void* send_data, int send_size, void* recv_data, int recv_size, int root) {
    MPI_Gather(send_data, send_size, MPI_BYTE, recv_data, recv_size, MPI_BYTE, root, MPI_COMM_WORLD);
}

static void barrier() {
    MPI_Barrier(MPI_COMM_WORLD);
}
