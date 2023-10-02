#include <algorithm>
#include <bits/types/time_t.h>
#include <mpi.h>
#include <fstream>
#include <iostream>
#include <mpi_proto.h>
#include <ratio>
#include <string>
#include <unistd.h>
#include <thread>
#include <ctime>

#include "wireless_sensor.h"
#include "types.h"
#include "mpi_helper.h"


WirelessSensor::WirelessSensor(int r_, int c_, int x_, int y_, int rank_, MPI_Comm ev_comm):
    row(r_), col(c_), x(x_), y(y_), rank(rank_), EV_comm(ev_comm),
    logger(LOG_PATH_PREFIX + std::to_string(rank_) + ".log")
{
    msg = new EVNodeMessage;
    msg->rank = rank_;
    this->get_neighbors();
    this->init_ports();
    this->full_log_num = 0;
    MPIHelper::create_EV_message_type(&EV_msg_type);
    stop = 0;

    std::thread report_thread(&WirelessSensor::report_availability, this);
    std::thread prompt_thread(&WirelessSensor::prompt_availability, this);
    std::thread respond_thread(&WirelessSensor::response_availability, this);
    std::thread listen_thread(&WirelessSensor::listen_terminal_from_base, this, row * col);
    std::thread port_threads[ports_num];
    for (int i = 0; i < ports_num; i++) {
        port_threads[i] = std::thread(&WirelessSensor::port_simulation, this, i);
    }

    report_thread.join();
    prompt_thread.join();
    respond_thread.join();
    listen_thread.join();
    for (int i = 0; i < ports_num; i++) {
        port_threads[i].join();
    }
}

WirelessSensor::~WirelessSensor()
{
    delete msg;
    MPI_Type_free(&EV_msg_type);
}

void WirelessSensor::get_neighbors()
{
    int dimension_sizes[2] = {row, col};    // 2-dim grid of Cart
    int periods[2] = {0, 0};
    MPI_Cart_create(EV_comm, 2, dimension_sizes, periods, 1, &grid_comm);
    /* get ranks of neighbours */
    /* get rank of top, bottom */
    MPI_Cart_shift(grid_comm, 0, 1, &msg->neighbor_ranks[0], &msg->neighbor_ranks[1]);
    /* get rank of left, right */
    MPI_Cart_shift(grid_comm, 1, 1, &msg->neighbor_ranks[2], &msg->neighbor_ranks[3]);
    
    msg->matching_neighbours = 0;
    /* get coordinations of neighbors */
    for (int i = 0; i < 4; ++i) {
        // avoid corner case
        if (msg->neighbor_ranks[i] != MPI_PROC_NULL) {
            msg->matching_neighbours++;
            MPI_Cart_coords(grid_comm, msg->neighbor_ranks[i], 2, msg->neighbor_coords[i]);
        }
    }
}

void WirelessSensor::init_ports()
{
    ports_avail.resize(ports_num);
    std::fill(ports_avail.begin(), ports_avail.end(), true);
}

void WirelessSensor::port_simulation(int port_id)
{
    while (!stop) {
        int time = rand() % 30;
        if (!ports_avail[port_id])
        {
            time += 60;
        }
        sleep(time);
        ports_avail[port_id] = !ports_avail[port_id];
    }
}

/**
 *  periodically updates and report the availability of node to shared array
*/
void WirelessSensor::report_availability()
{
    AvailabilityLog log_entry;
    time_t now;
    tm* ltm;
    char* ctm;
    int avail;
    
    while (!stop) {

        now = time(nullptr);
        ltm = localtime(&now);
        ctm = ctime(&now);
        avail = std::count_if(ports_avail.begin(), ports_avail.end(), [](bool i){return i;});
        log_entry.time.year = ltm->tm_year;
        log_entry.time.month = ltm->tm_mon;
        log_entry.time.day = ltm->tm_mday;
        log_entry.time.minute = ltm->tm_min;
        log_entry.time.second = ltm->tm_sec;
        log_entry.availability = avail;

        while (avail_table.size() >= FIXED_ARRAY_SIZE) 
        {
            avail_table.pop_front();
        }
        avail_table.push_back(log_entry);

        logger.avail_log(rank, ctm, avail);

        if (avail == 0) 
        {
            ++full_log_num;
        }

        sleep(AVAILABILITY_TIME_CYCLE);
    }
}

/**
 * If all ports (or almost all ports) are in full use, 
 * the node will prompt for neighbour node data.
*/
void WirelessSensor::prompt_availability()
{
    int avail_neighbor[4];
    int num_of_avail_neighbor;
    bool to_alert;

    while (!stop)
    {
        if (full_log_num > 0)
        {
            --full_log_num;
            logger.prompt_log(rank);
            get_message_from_neighbor(msg);
            to_alert = prompt_alert_or_not(msg, avail_neighbor, &num_of_avail_neighbor);
            if (to_alert) {
                send_alert_to_base(row * col, msg);
                listen_nearby_from_base(row * col);
            }
        }
    }
}

void WirelessSensor::response_availability()
{
    MPI_Request recv_reqs[4];
    MPI_Request send_reqs[4];
    MPI_Status stats[4];
    unsigned int my_avail[4];
    unsigned int avail = 0;
        
    for (int i = 0; i < 4; i++) {
        if (msg->neighbor_ranks[i] != MPI_PROC_NULL) {
            MPI_Irecv(&avail, 1, MPI_UNSIGNED, msg->neighbor_ranks[i], PROMPT_NEIGHBOR_MESSAGE, grid_comm, &recv_reqs[i]);
        }
    }

    /**
     *  if any recv_req is finished, send avail message to that source EVnode, 
     *  and start a new recv_req listening to that EVnode
    */
    while (!stop) {
        int flag = 0;
        for (int i = 0; i < 4; i++) {
            if (msg->neighbor_ranks[i] != MPI_PROC_NULL) {
                MPI_Test(&recv_reqs[i], &flag, &stats[i]);
                if (flag) {
                    my_avail[i] = avail_table.empty()? 0 : avail_table.back().availability;
                    MPI_Isend(&(my_avail[i]), 1, MPI_UNSIGNED, msg->neighbor_ranks[i], AVAIL_MESSAGE, grid_comm, &send_reqs[i]);
                    MPI_Irecv(&avail, 1, MPI_UNSIGNED, msg->neighbor_ranks[i], PROMPT_NEIGHBOR_MESSAGE, grid_comm, &recv_reqs[i]);
                }
            }
        }
    }
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
        if (msg->neighbor_avail_ports[i] > 0) {
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
    MPI_Request send_reqs[4];
    MPI_Request recv_reqs[4];
    MPI_Status send_stats[4];
    MPI_Status recv_stats[4];
    int valid_reqs_num = 0;

    for (int i = 0; i < 4; i++) {
        if (msg->neighbor_ranks[i] != MPI_PROC_NULL) {
            MPI_Isend(&(avail_table.back().availability), 1, MPI_UNSIGNED, msg->neighbor_ranks[i], PROMPT_NEIGHBOR_MESSAGE, grid_comm, &send_reqs[valid_reqs_num]);
            MPI_Irecv(&msg->neighbor_avail_ports[i], 1, MPI_UNSIGNED, msg->neighbor_ranks[i], AVAIL_MESSAGE, grid_comm, &recv_reqs[valid_reqs_num]);
            valid_reqs_num++;
        }
    }

    /* wait for result of MPI_Isend/MPI_Irecv */    
    MPI_Waitall(valid_reqs_num, send_reqs, send_stats);
    MPI_Waitall(valid_reqs_num, recv_reqs, recv_stats);

    for (int i = 0; i < 4; i++) {
        if (msg->neighbor_ranks[i] != MPI_PROC_NULL) {
            logger.neighbor_log(rank, msg->neighbor_ranks[i], msg->neighbor_avail_ports[i]);
        }
    }
}

void WirelessSensor::send_alert_to_base(int base_station_rank, EVNodeMessage* alert_msg) 
{   
    // single to single communication
    MPI_Request send_req;
    MPI_Status status;
    logger.alert_log(rank);
    alert_msg->alert_time = time(nullptr);
    MPI_Isend(alert_msg, 1, EV_msg_type, base_station_rank, ALERT_MESSAGE, MPI_COMM_WORLD, &send_req);
    MPI_Wait( &send_req , &status);
}

/**
 * Listening for a termination message from the base station,
 * once the node receives a termination message, the node cleans up and exits.
 */
void WirelessSensor::listen_terminal_from_base(int base_station_rank) {
    char buf[2];
    MPI_Status stat;
    MPI_Recv(buf, 1, MPI_CHAR, row * col, TERMINATE, MPI_COMM_WORLD, &stat);
    logger.terminate_log(rank);
    stop = 1;
}

/**
 * Listening for nearby nodes from the base station after the EVnode aberts
*/
void WirelessSensor::listen_nearby_from_base(int base_station_rank)
{
    int nearby_rank;
    MPI_Status stat;
    MPI_Recv(&nearby_rank, 1, MPI_INT, base_station_rank, NEARBY_AVAIL_MESSAGE, MPI_COMM_WORLD, &stat);
    logger.nearby_log(rank, nearby_rank);
}
