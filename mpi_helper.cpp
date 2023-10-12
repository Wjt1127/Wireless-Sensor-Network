#include <mpi.h>

#include "mpi_helper.h"
#include "ev_node.h"

/**
 * create MPI datatype for EVNodeMessage
*/
void MPIHelper::create_EV_message_type(MPI_Datatype *EV_message_type) {
    int num_of_fields = 7;
    int block_lengths[7] = {1, 1, 1, 4, 4 * 2, 4, 1};
    MPI_Aint displacements[7];
    displacements[0] = offsetof(EVNodeMessage, rank);
    displacements[1] = offsetof(EVNodeMessage, avail_ports);
    displacements[2] = offsetof(EVNodeMessage, matching_neighbours);
    displacements[3] = offsetof(EVNodeMessage, neighbor_ranks);
    displacements[4] = offsetof(EVNodeMessage, neighbor_coords);
    displacements[5] = offsetof(EVNodeMessage, neighbor_avail_ports);
    displacements[6] = offsetof(EVNodeMessage, alert_time);
    
    MPI_Datatype datatypes[7] = {
        MPI_INT,           MPI_INT,           MPI_INT,          MPI_INT,
        MPI_INT,           MPI_INT,           MPI_LONG
        };
    MPI_Type_create_struct(num_of_fields, block_lengths, displacements, datatypes, EV_message_type);
    MPI_Type_commit(EV_message_type);
}
