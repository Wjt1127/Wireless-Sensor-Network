#include <mpi.h>
#include <fstream>
#include <iostream>
#include <mpi_proto.h>
#include <unistd.h>
#include <thread>

#include "wireless_sensor.h"
#include "types.h"
#include "mpi_helper.h"


WirelessSensor::WirelessSensor(int r_, int c_, int x_, int y_, std::string &source):
    row(r_), col(c_), x(x_), y(y_)
{
    int dimension_sizes[2] = {row, col};    // 2-dim grid of Cart
    int periods[2] = {0, 0};

    MPI_Cart_create(EV_comm, 2, dimension_sizes, periods, 1, &grid_comm);
    MPIHelper::create_EV_message_type(&EV_msg_type);

    std::thread report_thread(&WirelessSensor::report_availability, this, source);
    std::thread prompt_thread(&WirelessSensor::prompt_availability, this);
    std::thread listen_thread(&WirelessSensor::listen_terminal_from_base, this);

    report_thread.join();
    prompt_thread.join();
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
    EVNodeMessage msg;
    int avail_neighbor[4];
    int num_of_avail_neighbor;
    bool to_alert;

    while (!avail_table.empty())
    {
        if (avail_table.back().availability == 0)
        {
            get_message_from_neighbor(&msg);
            to_alert = prompt_alert_or_not(&msg, avail_neighbor, &num_of_avail_neighbor);
            if (to_alert) {
                send_alert_to_base(row * col, &msg);
            }
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
    char buf[2];
    MPI_Status stat;
    MPI_Recv(buf, 1, MPI_CHAR, row * col, TERMINATE, MPI_COMM_WORLD, &stat);
    MPI_Finalize();
}

/**
 * Based on the number of available ports in the neighbouring nodes 
 * determine if a alert report should be prompted to the base station, 
 * if there are available ports then they are stored in the parameter avail_neighbor.
*/
bool WirelessSensor::prompt_alert_or_not(EVNodeMessage* msg, int avail_neighbor[], int* num_of_avail_neighbor) {
    bool isprompt = true;
    *num_of_avail_neighbor = 0;

    for (int i = 0; i < 4; i++) {
        if (msg->neighbor_ranks[i] == MPI_PROC_NULL) continue;
        if (msg->neighbor_avail_ports[i] < ports_num) {
            isprompt = false;
            avail_neighbor[(*num_of_avail_neighbor)++] = msg->neighbor_ranks[i];
        }
    }

    return isprompt;
}

/**
 * The node prompt for neighbor node data (top, bottom, right and left in 2-dims Cart),
 * neighbor nodes send data stored in msg.
 */
void WirelessSensor::get_message_from_neighbor(EVNodeMessage *msg) {

    int periods[2] = {0, 0};
    int reorder = 1;
    MPI_Request reqs[8];
    MPI_Status stats[8];

    /* get ranks of neighbours */
    /* get rank of top, bottom */
    MPI_Cart_shift(grid_comm, 0, 1, &msg->neighbor_ranks[0], &msg->neighbor_ranks[1]);
    /* get rank of left, right */
    MPI_Cart_shift(grid_comm, 1, 1, &msg->neighbor_ranks[2], &msg->neighbor_ranks[3]);
    
    /* get coordinations of neighbors */
    for (int i = 0; i < 4; ++i) {
        // avoid corner case
        if (msg->neighbor_ranks[i] != MPI_PROC_NULL) {
            MPI_Cart_coords(grid_comm, msg->neighbor_ranks[i], 2, msg->neighbor_coords[i]);
        }
    }

    for (int i = 0; i < 4; i++) {
        if (msg->neighbor_ranks[i] != MPI_PROC_NULL) {
            MPI_Isend(&(avail_table.back().availability), 1, MPI_UNSIGNED, msg->neighbor_ranks[i], AVAIL_MESSAGE, grid_comm, &reqs[i]);
            MPI_Irecv(&msg->neighbor_avail_ports[i], 1, MPI_UNSIGNED, msg->neighbor_ranks[i], AVAIL_MESSAGE, grid_comm, &reqs[i + 4]);
        }
    }
    /* wait for result of MPI_Isend/MPI_Irecv */
    MPI_Waitall(8, reqs, stats);
}

void WirelessSensor::send_alert_to_base(int base_station_rank, EVNodeMessage* alert_msg) 
{   
    // single to single communication
    MPI_Send(alert_msg, 1, EV_msg_type, base_station_rank, ALERT_MESSAGE, MPI_COMM_WORLD);
}