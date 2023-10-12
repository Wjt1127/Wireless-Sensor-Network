#ifndef MPI_HELPER_H
#define MPI_HELPER_H

#include <mpi.h>

class MPIHelper
{
public:
    static void create_EV_message_type(MPI_Datatype *EV_message_type);
};


#endif