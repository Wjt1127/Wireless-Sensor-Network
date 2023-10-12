#ifndef MPI_HELPER_H
#define MPI_HELPER_H

#include <bits/types/time_t.h>
#include <mpi.h>
#include <mpi_proto.h>
#include <string>

typedef struct {
    int rank;
    int avail_ports;
    int matching_neighbours;  // size of neighbour(if the node at edge, matching_neighbours may be 2 or 3)
    int neighbor_ranks[4];  // rank of neighbours
    int neighbor_coords[4][2]; // 2-dims coordinations of neighbour
    int neighbor_avail_ports[4]; // the available port num of neighbor ports

    time_t alert_time;        // when the event occured
} EVNodeMesg;

class MPIHelper
{
public:
    static void create_EV_message_type(MPI_Datatype *EV_message_type)
    {
        int num_of_fields = 7;
        int block_lengths[7] = {1, 1, 1, 4, 4 * 2, 4, 1};
        MPI_Aint displacements[7];
        displacements[0] = offsetof(EVNodeMesg, rank);
        displacements[1] = offsetof(EVNodeMesg, avail_ports);
        displacements[2] = offsetof(EVNodeMesg, matching_neighbours);
        displacements[3] = offsetof(EVNodeMesg, neighbor_ranks);
        displacements[4] = offsetof(EVNodeMesg, neighbor_coords);
        displacements[5] = offsetof(EVNodeMesg, neighbor_avail_ports);
        displacements[6] = offsetof(EVNodeMesg, alert_time);
        
        MPI_Datatype datatypes[7] = {
            MPI_INT,           MPI_INT,           MPI_INT,          MPI_INT,
            MPI_INT,           MPI_INT,           MPI_LONG
            };
        MPI_Type_create_struct(num_of_fields, block_lengths, displacements, datatypes, EV_message_type);
        MPI_Type_commit(EV_message_type);
    }
};

#define SEND_ALERT_SUCCESS  1
#define SEND_ALERT_FAIL     0

enum {
    AVAIL_MESSAGE = 1,
    ALERT_MESSAGE = 2,
    PROMPT_NEIGHBOR_MESSAGE = 3,
    NEARBY_AVAIL_MESSAGE = 4,
    TERMINATE = 5,
};

#endif