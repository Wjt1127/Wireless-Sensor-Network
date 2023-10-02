#include <mpi.h>

#include "mpi_helper.h"
#include "wireless_sensor.h"

/* send to single node */
void MPIHelper::send_method(const void *buf, int count, MPI_Datatype datatype, int dest_rank, MPI_Comm comm) {
    // single to single communication
    MPI_Send(buf, count, datatype, dest_rank, 0, MPI_COMM_WORLD);
}

void MPIHelper::recv_method(int neighbor_ranks[4], unsigned int available_port, int neighbor_avail_ports[4], MPI_Comm comm) {
    /* get coordinations of neighbors */
    MPI_Request reqs[8];
    MPI_Status stats[8];
    for (int i = 0; i < 4; i++) {
        if (neighbor_ranks[i] != MPI_PROC_NULL) {
            MPI_Isend(&available_port, 1, MPI_UNSIGNED, neighbor_ranks[i], 1, comm, &reqs[i]);
            MPI_Irecv(&neighbor_avail_ports[i], 1, MPI_UNSIGNED, neighbor_ranks[i], 1, comm, &reqs[i + 4]);
        }
    }
    /* wait for result of MPI_Isend/MPI_Irecv */
    MPI_Waitall(8, reqs, stats);
}

/**
 * create MPI datatype for EVNodeMessage
*/
void MPIHelper::create_EV_message_type(MPI_Datatype *EV_message_type) {
    int num_of_fields = 14;
    int block_lengths[14] = {1, 1, 4, 4 * 2, 4};
    MPI_Aint displacements[14];
    displacements[0] = offsetof(EVNodeMessage, rank);
    displacements[1] = offsetof(EVNodeMessage, matching_neighbours);
    displacements[2] = offsetof(EVNodeMessage, neighbor_ranks);
    displacements[3] = offsetof(EVNodeMessage, neighbor_coords);

    /* wait for complete */
    
    MPI_Datatype datatypes[14] = {
        MPI_INT,           MPI_INT,           MPI_INT,
        MPI_INT,           MPI_INT};
    MPI_Type_create_struct(num_of_fields, block_lengths, displacements,
                           datatypes, EV_message_type);
    MPI_Type_commit(EV_message_type);
}
