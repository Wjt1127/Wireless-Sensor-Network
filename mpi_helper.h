#ifndef MPI_HELPER_H
#define MPI_HELPER_H

#include <mpi.h>

class MPIHelper
{
public:
    static void send_method(const void *buf, int count, MPI_Datatype datatype, int dest_rank, MPI_Comm comm);
    static void recv_method(int neighbor_ranks[4], unsigned int available_port, int neighbor_avail_ports[4], MPI_Comm comm);
    static void create_EV_message_type(MPI_Datatype *EV_message_type);

};


#endif