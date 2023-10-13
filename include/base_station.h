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

#define BS_LOG_FILE "./bs_log/bstation.log"

typedef struct {
    EVNodeMesg msg;
    timeval log_t;
    int log_iteration;
} BS_log;

class BStation {
public:
    BStation() = default;
    BStation(unsigned int iteration_interval_, unsigned int total_iter, int row_, int col_);

private:
    int row;
    int col;
    /* interval time of a iteration (s) */
    unsigned int iteration_interval;
    /* total num of iterations to simulate */
    unsigned int total_iter; 
    // current iteration */
    std::atomic_uint now_iter;
    int bs_rank;
    /* alert events happen in a term */
    int alert_events = 0;
    int full_threshold = 1;
    /* hashmap : (rank, iteration) -> send alert */
    std::unordered_map<long long, bool> send_alert;
    MPILogger logger;
    MPI_Datatype EV_msg_type;

    void listen_alert(int *alert_events);
    void iteration_timer();
    void send_ternimate();

    bool check_evnode_avail(int rank, int iter);
    void get_available_EVNodes(EVNodeMesg* msg, int *node_list, int *num_of_list, int cur_iter);
    void get_neighbor_rank(int rank, int *adjacent_rank);
    void process_alert(BS_log &alert);
    
    void do_alert_log(EVNodeMesg* msg, timeval log_time, int *nearby_avail_nodes, int num_of_avail, int now_iter);
    
};

#endif
