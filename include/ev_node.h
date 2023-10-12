#ifndef WIRELESS_SENSOR_H
#define WIRELESS_SENSOR_H

#include <atomic>
#include <stdio.h>
#include <fstream>
#include <string>
#include <deque>
#include <vector>

#include "mpi_helper.h"
#include "ev_logger.h"

#define MAX_DATA_LENGTH 1024
#define AVAILABILITY_TIME_CYCLE 10
#define FIXED_ARRAY_SIZE 512
#define LOG_PATH_PREFIX "./logs/evnode"

struct AvailabilityLog
{
    time_t timestamp;
    unsigned int availability;
};

class EVNode
{
public:
    EVNode() = delete;
    EVNode(int r_, int c_, int x_, int y_, int rank_, MPI_Comm ev_comm);
    ~EVNode();

private:
    int row;
    int col;
    int x;
    int y;
    int rank;
    int ports_num = 5;
    int full_threshold = 1;
    std::vector<int> ports_avail;
    std::deque<AvailabilityLog> avail_table;
    MPI_Comm EV_comm;
    MPI_Comm grid_comm;
    MPI_Datatype EV_msg_type;
    EVNodeMesg *msg;
    std::atomic_int full_log_num;
    std::atomic_int stop;
    MPILogger logger;
    
    void init_neighbors();
    void init_ports();

    void port_simulation(int port_id);
    void send_prompt();
    void send_alert(int bs_rank);
    
    void receive_message();
    void proccess_prompt(int source);
    void proccess_neighbor_availability(int source, std::atomic_int *responsed);
    void process_terminate(int bs_rank);
    void process_nearby(int bs_rank);

    bool alert_or_not(EVNodeMesg* msg);

};

#endif
