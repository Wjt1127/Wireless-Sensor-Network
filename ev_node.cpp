#include <algorithm>
#include <bits/types/time_t.h>
#include <sys/time.h>
#include <mpi.h>
#include <fstream>
#include <iostream>
#include <mpi_proto.h>
#include <ratio>
#include <string>
#include <unistd.h>
#include<stdlib.h>
#include <thread>
#include <ctime>

#include "ev_node.h"
#include "types.h"
#include "mpi_helper.h"


EVNode::EVNode(int r_, int c_, int x_, int y_, int rank_, MPI_Comm ev_comm):
    row(r_), col(c_), x(x_), y(y_), rank(rank_), EV_comm(ev_comm),
    logger(LOG_PATH_PREFIX + std::to_string(rank_) + ".log")
{
    msg = new EVNodeMessage;
    msg->rank = rank_;
    this->init_neighbors();
    this->init_ports();
    this->full_log_num = 0;
    MPIHelper::create_EV_message_type(&EV_msg_type);
    stop = 0;

    std::thread prompt_thread(&EVNode::send_prompt, this);
    std::thread listen_thread(&EVNode::receive_message, this);
    std::thread port_threads[ports_num];
    for (int i = 0; i < ports_num; i++) {
        port_threads[i] = std::thread(&EVNode::port_simulation, this, i);
    }

    // report_thread.join();
    prompt_thread.join();
    listen_thread.join();
    for (int i = 0; i < ports_num; i++) {
        port_threads[i].join();
    }
}

EVNode::~EVNode()
{
    delete msg;
    MPI_Type_free(&EV_msg_type);
    MPI_Comm_free(&grid_comm);
}

void EVNode::init_neighbors()
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

void EVNode::init_ports()
{
    ports_avail.resize(ports_num);
    std::fill(ports_avail.begin(), ports_avail.end(), true);
}

/**
 *  periodically updates and report the availability of node to shared array
*/
void EVNode::port_simulation(int port_id)
{
    AvailabilityLog log_entry;
    int avail;

    // stagger different mpi processes
    srand(getpid());
    while (!stop) {
        if (port_id == 0) {
            log_entry.timestamp = time(nullptr);
            avail = std::count_if(ports_avail.begin(), ports_avail.end(), [](bool i){return i;});
            log_entry.availability = avail;
            if (avail_table.size() >= FIXED_ARRAY_SIZE)
            {
                avail_table.pop_front();
            }
            avail_table.push_back(log_entry);

            logger.avail_log(rank, ctime(&log_entry.timestamp), avail);

            if (avail <= consider_full) 
            {
                msg->avail_ports = avail;
                ++full_log_num;
            }
        }

        sleep(AVAILABILITY_TIME_CYCLE);
        ports_avail[port_id] = rand() % 2;
    }
}

/**
 * If all ports (or almost all ports) are in full use, 
 * the node will prompt for neighbour node data.
*/
void EVNode::send_prompt()
{
    unsigned int avail;

    while (!stop)
    {
        if (full_log_num > 0)
        {
            --full_log_num;
            logger.prompt_log(rank);
            avail = 0;
            for (int i = 0; i < 4; i++) {
                if (msg->neighbor_ranks[i] != MPI_PROC_NULL) {
                    MPI_Send(&avail, 1, MPI_UNSIGNED, msg->neighbor_ranks[i], PROMPT_NEIGHBOR_MESSAGE, grid_comm);
                }
            }
        }
    }
}

/**
 *  if any recv_req is finished, send avail message to that source EVnode
*/
void EVNode::proccess_prompt(int source)
{
    MPI_Status stat;
    unsigned int my_avail;
    int avail;
    
    MPI_Recv(&avail, 1, MPI_UNSIGNED, source, PROMPT_NEIGHBOR_MESSAGE, grid_comm, &stat);
    
    if (avail_table.empty()) {
        my_avail = 0;
    }
    else {
        AvailabilityLog log = avail_table.back();
        my_avail = log.availability;
    }

    // printf("neighbor %d: avail is %d\n", msg->neighbor_ranks[i], avail);
    MPI_Send(&my_avail, 1, MPI_UNSIGNED, source, AVAIL_MESSAGE, grid_comm);
    // printf("neighbor %d: sended prompt to %d\n", msg->neighbor_ranks[i], rank);
                
}

void EVNode::proccess_neighbor_availability(int source, std::atomic_int *responsed)
{
    MPI_Status stat;

    for (int i = 0; i < 4; i++) {
        if (msg->neighbor_ranks[i] == source) {
            MPI_Recv(&msg->neighbor_avail_ports[i], 1, MPI_UNSIGNED, msg->neighbor_ranks[i], AVAIL_MESSAGE, grid_comm, &stat);
            logger.neighbor_log(rank, msg->neighbor_ranks[i], msg->neighbor_avail_ports[i]);
            ++(*responsed);
            break;
        }
    }
    if ((*responsed) == msg->matching_neighbours) {
        bool is_alert = alert_or_not(msg);
        if (is_alert) {
            send_alert(row * col);
        }
        (*responsed) = 0;
    }
}

/**
 * Based on the number of available ports in the neighbouring nodes 
 * determine if a alert report should be prompted to the base station, 
 * if there are available ports then they are stored in the parameter avail_neighbor.
*/
bool EVNode::alert_or_not(EVNodeMessage* msg) {
    bool is_prompt = true;

    for (int i = 0; i < 4; i++) {
        if (msg->neighbor_ranks[i] == MPI_PROC_NULL) continue;
        if (msg->neighbor_avail_ports[i] > 0) {
            is_prompt = false;
        }
    }

    return is_prompt;
}

void EVNode::send_alert(int base_station_rank) 
{   
    // single to single communication
    EVNodeMessage alert_msg = *msg;
    // MPI_Request req;
    logger.alert_log(rank);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    alert_msg.alert_time_s = tv.tv_sec;
    alert_msg.alert_time_us = tv.tv_usec;
    // MPI_Isend(&alert_msg, 1, EV_msg_type, base_station_rank, ALERT_MESSAGE, MPI_COMM_WORLD, &req);
    MPI_Send(&alert_msg, 1, EV_msg_type, base_station_rank, ALERT_MESSAGE, MPI_COMM_WORLD);
}

void EVNode::receive_message()
{
    int base_station = row * col;
    int flag;
    MPI_Status stat;
    std::atomic_int responsed(0);
    
    while (!stop) {

        MPI_Iprobe(base_station, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &stat);
        if (flag) {
            if (stat.MPI_TAG == TERMINATE) {
                process_terminate(base_station);
            }
            else if (stat.MPI_TAG == NEARBY_AVAIL_MESSAGE) {
                process_nearby(base_station);
            }
        }

        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, grid_comm, &flag, &stat);
        if (flag) {
            if (stat.MPI_TAG == PROMPT_NEIGHBOR_MESSAGE) {
                proccess_prompt(stat.MPI_SOURCE);
            }
            else if (stat.MPI_TAG == AVAIL_MESSAGE) {
                proccess_neighbor_availability(stat.MPI_SOURCE, &responsed);
            }
        }
    }
}

/**
 * Listening for a termination message from the base station,
 * once the node receives a termination message, the node cleans up and exits.
 */
void EVNode::process_terminate(int base_station_rank) {
    char buf;
    MPI_Status status;

    MPI_Recv(&buf, 1, MPI_CHAR, row * col, TERMINATE, MPI_COMM_WORLD, &status);
    logger.terminate_log(rank);
    stop = 1;
}

/**
 * Listening for nearby nodes from the base station after the EVnode aberts
*/
void EVNode::process_nearby(int base_station_rank)
{
    int nearby_rank;
    MPI_Status stat;
    MPI_Recv(&nearby_rank, 1, MPI_INT, base_station_rank, NEARBY_AVAIL_MESSAGE, MPI_COMM_WORLD, &stat);
    
    logger.nearby_log(rank, nearby_rank);
}
