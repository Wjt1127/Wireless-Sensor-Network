#include <mpi.h>
#include "mpi_helper.h"

/* send to single node */
void MPIHelper::send_method(const void *buf, int count, MPI_Datatype datatype, int dest_rank, MPI_Comm comm) {
    // single to single communication
    MPI_Send(buf, count, datatype, dest_rank, 0, MPI_COMM_WORLD);
}

void MPIHelper::recv_method(int neighbor_ranks[4], unsigned int available_port, int neighbor_avail_ports[4], MPI_Comm comm) {
    /* get coordinations of neighbors */
    MPI_Request reqs[8];
    for (int i = 0; i < 4; i++) {
        if (neighbor_ranks[i] != MPI_PROC_NULL) {
            MPI_Isend(&available_port, 1, MPI_UNSIGNED, neighbor_ranks[i], 1, comm, &reqs[i]);
            MPI_Irecv(&neighbor_avail_ports[i], 1, MPI_UNSIGNED, neighbor_ranks[i], 1, comm, &reqs[i + 4]);
        }
    }
}