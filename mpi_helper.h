#include <mpi.h>

class MPIHelper
{
public:
    void send_method(const void *buf, int count, MPI_Datatype datatype, int dest_rank, MPI_Comm comm);
    void recv_method(int neighbor_ranks[4], unsigned int available_port, int neighbor_avail_ports[4], MPI_Comm comm);
};