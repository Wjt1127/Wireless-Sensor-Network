#include <mpi.h>
#include <iostream>
#include <mpi_proto.h>
#include <unistd.h>
#include <sched.h>
#include <stdlib.h>
#include "base_station.h"
#include "wireless_sensor.h"


int main(int argc, char *argv[]) {
    int row, col, world_rank, numprocs;
    int iter_interval, iter_num;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    char *strrow, *strcol, *strint, *strnum;
    row = (int)strtol(argv[1], &strrow, 10);
    col = (int)strtol(argv[2], &strcol, 10);
    iter_interval = (int)strtol(argv[3], &strint, 10);
    iter_num = (int)strtol(argv[4], &strnum, 10);

    MPI_Comm EV_comm;
    MPI_Comm_split(MPI_COMM_WORLD, world_rank == numprocs - 1, 0, &EV_comm); // EV_comm is comunicator in WSN

    if (world_rank == numprocs - 1) {
        // start a base station
        printf("rank is %d\t\tpid is %d\n", world_rank, getpid());
        BStation base_station(iter_interval, iter_num, row, col);
    }
    else {
        // start a sensor
        printf("rank is %d\t\tpid is %d\n", world_rank, getpid());
        WirelessSensor WirelessSensor(row, col, world_rank / col, world_rank % col, world_rank, EV_comm);
    }
    
    MPI_Comm_free(&EV_comm);
    MPI_Finalize();
    return 0;
}