#include <mpi.h>
#include <iostream>
#include <mpi_proto.h>
#include <unistd.h>
#include <sched.h>
#include <stdlib.h>
#include "base_station.h"
#include "ev_node.h"


int main(int argc, char *argv[]) {
    int world_rank, numprocs;
    int row, col, iter_interval, iter_num, port_num;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    row = atoi(argv[1]);
    col = atoi(argv[2]);
    iter_interval = atoi(argv[3]);
    iter_num = atoi(argv[4]);
    port_num = atoi(argv[5]);

    MPI_Comm ev_comm;
    MPI_Comm_split(MPI_COMM_WORLD, world_rank == numprocs - 1, 0, &ev_comm); // ev_comm is comunicator in WSN

    if (world_rank == numprocs - 1) {
        // start a base station
        BStation base_station(iter_interval, iter_num, row, col);
    }
    else {
        // start a sensor
        EVNode ev_node(row, col, world_rank / col, world_rank % col, world_rank, port_num, ev_comm);
    }
    
    MPI_Comm_free(&ev_comm);
    MPI_Finalize();
    return 0;
}