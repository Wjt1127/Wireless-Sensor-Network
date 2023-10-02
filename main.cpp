#include <mpi.h>
#include <iostream>
#include <mpi_proto.h>
#include <stdlib.h>
#include "base_station.h"
#include "wireless_sensor.h"


int main(int argc, char *argv[]) {
    int row, col, world_rank, numprocs;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    char *strrow, *strcol;
    row = (int)strtol(argv[1], &strrow, 10);
    col = (int)strtol(argv[2], &strcol, 10);

    MPI_Comm EV_comm;
    MPI_Comm_split(MPI_COMM_WORLD, world_rank == numprocs - 1, 0, &EV_comm); // EV_comm is comunicator in WSN

    if (world_rank == numprocs - 1) {
        // start a base station
        BStation base_station;
    }
    else {
        // start a sensor
        std::string source;
        WirelessSensor WirelessSensor(row, col, world_rank / col, world_rank % col, source);
    }
    
    MPI_Finalize();
    return 0;
}