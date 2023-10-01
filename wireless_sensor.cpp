#include <mpi.h>
#include <fstream>
#include <iostream>
#include <mpi_proto.h>
#include <unistd.h>
#include <thread>

#include "wireless_sensor.h"
#include "types.h"


WirelessSensor::WirelessSensor(int r_, int c_, int x_, int y_, std::string &source):
    row(r_), col(c_), x(x_), y(y_)
{
    int grid_dimensions = 2;
    int dimension_sizes[2] = {row, col};    // 2-dim grid of Cart
    MPI_Comm grid_comm;
    int periods[2] = {0, 0};
    int reorder = 1;
    MPI_Request reqs[8]; 

    MPI_Cart_create(EV_Comm, grid_dimensions, dimension_sizes, periods, reorder, &grid_comm);
    
    std::thread report_thread(&WirelessSensor::report_availability, this, source);
    std::thread prompt_thread(&WirelessSensor::prompt_availability, this);
    std::thread listen_thread(&WirelessSensor::listen_terminal_from_base, this);
    report_thread.join();
    listen_thread.join();
}

/**
 *  periodically updates and report the availability of node to shared array
*/
void WirelessSensor::report_availability(std::string avail_source)
{
    std::ifstream avail_stream;

    avail_stream.open(avail_source);
    if (!avail_stream.is_open()) 
    {
        std::cout << avail_source + " open failed" << std::endl;
        return;
    }

    while (!avail_stream.eof())
    {
        sleep(AVAILABILITY_TIME_CYCLE);
        AvailabilityLog log_entry;
        avail_stream >> log_entry;

        while (avail_table.size() >= FIXED_ARRAY_SIZE) 
        {
            avail_table.pop_front();
        }
        avail_table.push_back(log_entry);
    }
}

/**
 * If all ports (or almost all ports) are in full use, 
 * the node will prompt for neighbour node data.
*/
void WirelessSensor::prompt_availability()
{

    while (!avail_table.empty())
    {
        if (avail_table.back().availability == 0)
        {
            
        }
    }
}

/**
 * Listening for a termination message from the base station,
 * once the node receives a termination message, the node cleans up and exits.
 */
void WirelessSensor::listen_terminal_from_base(int base_station_rank) {
    /*
        need to be done
    */

}

/**
 * Based on the number of available ports in the neighbouring nodes 
 * determine if a alert report should be prompted to the base station, 
 * if there are available ports then they are stored in the parameter avail_neighbor.
*/
bool WirelessSensor::prompt_alert_or_not(EVNodeMessage* msg, int avail_neighbor[], int num_of_avail_neighbor) {
    bool isprompt = true;
    num_of_avail_neighbor = 0;

    for (int i = 0; i < 4; i++) {
        if (msg->neighbor_ranks[i] == MPI_PROC_NULL) continue;
        if (msg->neighbor_avail_ports[i] < ports_num) {
            isprompt = false;
            avail_neighbor[num_of_avail_neighbor++] = msg->neighbor_ranks[i];
        }
    }

    return isprompt;
}

/**
 * The node prompt for neighbor node data (top, bottom, right and left in 2-dims Cart),
 * neighbor nodes send data stored in msg.
 */
void WirelessSensor::get_message_from_neighbor(MPI_Comm EV_Comm, EVNodeMessage *msg) {
    /* single to group in Cart */

    int grid_dimensions = 2;
    int dimension_sizes[2] = {row, col};    // 2-dim grid of Cart
    MPI_Comm grid_comm;
    int periods[2] = {0, 0};
    int reorder = 1;
    MPI_Request reqs[8]; 

    MPI_Cart_create(EV_Comm, grid_dimensions, dimension_sizes, periods, reorder, &grid_comm);
    /* get ranks of neighbours */
    MPI_Cart_shift(grid_comm, 0, 1, &msg->neighbor_ranks[0], &msg->neighbor_ranks[1]);  // get rank of top, bottom 
    MPI_Cart_shift(grid_comm, 1, 1, &msg->neighbor_ranks[2], &msg->neighbor_ranks[3]);  // get rank of left, right
    
    /* get coordinations of neighbors */
    for (int i = 0; i < 4; ++i) {
        // avoid corner case
        if (msg->neighbor_ranks[i] != MPI_PROC_NULL)
            MPI_Cart_coords(grid_comm, msg->neighbor_ranks[i], grid_dimensions, msg->neighbor_coords[i]);
    }

    for (int i = 0; i < 4; i++) {
        if (msg->neighbor_ranks[i] != MPI_PROC_NULL) {
            MPI_Isend(&(avail_table.back().availability), 1, MPI_UNSIGNED, msg->neighbor_ranks[i], 1, grid_comm, &reqs[i]);
            MPI_Irecv(&msg->neighbor_avail_ports[i], 1, MPI_UNSIGNED, msg->neighbor_ranks[i], 1, grid_comm, &reqs[i + 4]);
        }
    }

}

void WirelessSensor::send_alert_to_base(int base_station_rank, char* alert_msg) 
{   
    // single to single communication
    MPI_Send(alert_msg, sizeof(alert_msg), MPI_CHAR, base_station_rank, 0, MPI_COMM_WORLD);
}