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

#define BS_LOG_FILE "./logs/bstation.log"

typedef struct {
    EVNodeMesg msg;
    timeval log_t;
    int log_iteration;
} BS_log;

class BStation {
public:
    BStation() = default;
    BStation(unsigned int _iteration_interval, unsigned int total_iter, int _row, int _col);

private:
    /* interval time of a iteration (s) */
    unsigned int iteration_interval;
    /* total num of iterations to simulate */
    unsigned int total_iter; 
    // current iteration */
    std::atomic_uint now_iter;
    int bs_rank;
    int row;
    int col;
    /* alert events happen in a term */
    int alert_events = 0;
    int full_threshold = 1;
    /* hashmap : (rank, iteration) -> send alert */
    std::unordered_map<long long, bool> send_alert;
    MPILogger logger;
    FILE* log_fp;
    MPI_Datatype EV_msg_type;

    void process_alert(BS_log &alert);
    void listen_alert(int *alert_events);
    void get_available_EVNodes(EVNodeMesg* msg, int *node_list, int *num_of_list, int cur_iter);
    void send_ternimate_signal();

    void iteration_recorder();
    void get_neighbor_coord_from_rank(int rank, int adjacent_coords[][2]);
    void get_neighbor_rank(int rank, int *adjacent_rank);
    
    void do_alert_log(EVNodeMesg* msg, timeval log_time, int *nearby_avail_nodes, int num_of_avail, int now_iter);
    bool check_evnode_avail(int rank, int iter);
};



#endif
