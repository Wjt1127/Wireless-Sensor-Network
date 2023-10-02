#ifndef BASE_STATION_H
#define BASE_STATION_H

#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <vector>

#include "wireless_sensor.h"


class BStation {
public:
    BStation() = default;
    BStation(unsigned int _iteration_interval, unsigned int iterations_num, int _row, int _col);
    ~BStation();

private:
    unsigned int iteration_interval;    // interval time of a iteration (s)
    unsigned int iterations_num;        // the num of iterations
    unsigned int cur_iteration;         // iteration-thread updates cur_iteration
    int Base_station_rank;
    int row;
    int col;
    int alert_events;                   // alert events happen in a term
    std::vector<bool> nodes_avail;      // each EV node is available(true) or not
    std::vector<std::pair<EVNodeMessage *, double> > alert_msgs;    // store alert messages and log time in an iteration 
    FILE* log_fp;
    MPI_Datatype EV_msg_type;
    const char* LOG_FILE = "base_station.log";

    void BStation_run();

    void process_alert_report(EVNodeMessage* msg, double recv_time);
    void listen_report_from_WSN(int *alert_events);
    void get_available_EVNodes(EVNodeMessage* msg, int *node_list, int *num_of_list);
    void send_ternimate_signal(int dest_rank);

    void init_nodes_avail();
    int cal_sleep_time(double start_time);
    void iteration_recorder();
    void get_neighbor_coord_from_rank(int rank, int adjacent_coords[][2]);
    void get_neighbor_rank(int rank, int *adjacent_rank);
};



#endif