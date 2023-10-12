#ifndef BASE_STATION_H
#define BASE_STATION_H

#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <cstring>
#include <sys/stat.h>
#include <time.h>

#include <atomic>
#include <vector>
#include <deque>
#include <set>
#include <unordered_map>
#include <mutex>

#include "ev_node.h"
#include "mpi_helper.h"
#include "ring_queue_helper.h"

typedef struct {
    EVNodeMessage msg;
    timeval log_t;
    int log_iteration;
} BS_log;

class BStation {
public:
    BStation() = default;
    BStation(unsigned int _iteration_interval, unsigned int iterations_num, int _row, int _col);
    ~BStation();

private:
    unsigned int iteration_interval;    // interval time of a iteration (s)
    unsigned int iterations_num;        // the num of iterations
    std::atomic<unsigned int> cur_iteration;         // iteration-thread updates cur_iteration
    int Base_station_rank;
    int row;
    int col;
    int alert_events = 0;                   // alert events happen in a term
    std::atomic<int> iprobe_count;
    int consider_full = 1;

    std::unordered_map<long long, bool> send_alert;      // {rank, iteration} -> send alert
    CircularQueue<BS_log> alert_msgs;   // store alert messages and log time in an iteration 
    FILE* log_fp;
    MPI_Datatype EV_msg_type;
    const char* LOG_FILE = "./logs/bstation.log";

    void process_alert_report();
    void listen_report_from_WSN(int *alert_events);
    void get_available_EVNodes(EVNodeMessage* msg, int *node_list, int *num_of_list, int cur_iter);
    void send_ternimate_signal();

    void iteration_recorder();
    void get_neighbor_coord_from_rank(int rank, int adjacent_coords[][2]);
    void get_neighbor_rank(int rank, int *adjacent_rank);
    
    void do_alert_log(EVNodeMessage* msg, timeval log_time, int *nearby_avail_nodes, int num_of_avail, int cur_iteration);
    void print_log(std::string info);
    bool check_last_3iter(int rank, int iter);
};



#endif
