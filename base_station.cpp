#include <bits/types/time_t.h>
#include <cstddef>
#include <cstdio>
#include <mpi.h>
#include <mpi_proto.h>
#include <string>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include "base_station.h"
#include "types.h"
#include "wireless_sensor.h"
#include "mpi_helper.h"

BStation::BStation(unsigned int _iteration_interval, unsigned int _iterations_num, int _row, int _col) : 
    iteration_interval(_iteration_interval), iterations_num(_iterations_num), row(_row), col(_col) {
    
    cur_iteration = 0;
    Base_station_rank = row * col;
    alert_events = 0;

    log_fp = fopen(LOG_FILE, "a");
    if (log_fp == NULL) {
        printf( "Could not open file %s\n", LOG_FILE);
        exit(-1);
    }
    
    MPIHelper::create_EV_message_type(&EV_msg_type);
    std::thread listen_thread(&BStation::listen_report_from_WSN, this, &alert_events);
    std::thread timer_thread(&BStation::iteration_recorder, this);
    std::thread process_alert_thread(&BStation::process_alert_report, this);

    timer_thread.join();
    listen_thread.join();
    process_alert_thread.join();
}

BStation::~BStation() {
    if (log_fp != NULL) fclose(log_fp);
}

void BStation::iteration_recorder() {
    while (cur_iteration < iterations_num) {
        sleep(iteration_interval);
        
        std::fill(nodes_avail.begin(), nodes_avail.end(), true);
        cur_iteration++;
    }
}

void BStation::listen_report_from_WSN(int *alert_events) {
    time_t recv_time;
    MPI_Request recv_reqs[Base_station_rank];
    EVNodeMessage msg[Base_station_rank];
    MPI_Status stats[Base_station_rank];
    int flag = 0;

    for (int i = 0; i < Base_station_rank; i++) {
        MPI_Irecv(&msg[i], 1, EV_msg_type, i, ALERT_MESSAGE, MPI_COMM_WORLD, &recv_reqs[i]);
    }

    while (cur_iteration < iterations_num) {
        // check if a EVNode has sent an alert message
        flag = 0;
        for (int i = 0; i < Base_station_rank; i++) {
            MPI_Test(&recv_reqs[i], &flag, &stats[i]);
            if (flag) {
                recv_time = time(nullptr);
                BS_log alert_log;
                alert_log.msg = msg;
                alert_log.log_t = recv_time;
                alert_log.log_iteration = cur_iteration;
                alert_msgs.push_back(alert_log);

                MPI_Irecv(&msg[i], 1, EV_msg_type, i, ALERT_MESSAGE, MPI_COMM_WORLD, &recv_reqs[i]);
            }
        }
    }

    // send ternimate signal to all EVNode
    for (int dest_rank = 0; dest_rank < Base_station_rank; dest_rank++)
        send_ternimate_signal(dest_rank);

    if (log_fp != NULL) {
        printf("\n%d iterations reached, terminating\n", iterations_num);
    } 
    else {
        printf("\nLog file cannot be detected, terminating\n");
    }

    fclose(log_fp);

    MPI_Type_free(&EV_msg_type);
    MPI_Finalize();
}

/**
 * send ternimate signal to all EVnode
*/
void BStation::send_ternimate_signal(int dest_rank) {
    char buf = '\0';
    // send message to terminate
    MPI_Send(&buf, 1, MPI_CHAR, dest_rank, TERMINATE, MPI_COMM_WORLD);
}


/**
 * get nearby available nodes
*/
void BStation::get_available_EVNodes(EVNodeMessage* msg, int *node_list, int *num_of_list) {
    for (int i = 0; i < 4; ++i) {
        int neighbor_rank = msg->neighbor_ranks[i];
        if (neighbor_rank != MPI_PROC_NULL) {
            // the adjacent nodes of the neighbouring nodes
            int adjacent_rank[4];
            get_neighbor_rank(neighbor_rank, adjacent_rank);
            for (int j = 0; j < 4; j++) {
                if (adjacent_rank[j] == -1) continue;
                if (nodes_avail[adjacent_rank[j]] == true) {
                    node_list[(*num_of_list)++] = adjacent_rank[j];
                }
            }
        }
    }
}

void BStation::get_neighbor_coord_from_rank(int rank, int adjacent_coords[][2]) {
    int r = rank / col, c = rank % col;
    int direction[4][2] = { {1,0}, {0, 1}, {-1, 0}, {0, -1}};
    for (int i = 0; i < 4; i++) {
        int next_rank = rank + direction[i][0] * col + direction[i][1];
        int next_row = r + direction[i][0], next_col = c + direction[i][1];
        if (next_rank < 0 || next_rank >= Base_station_rank) {
            adjacent_coords[i][0] = -1;
            adjacent_coords[i][0] = -1;
        }
        else {
            adjacent_coords[i][0] = next_row;
            adjacent_coords[i][1] = next_col;
        }
    }
}


void BStation::get_neighbor_rank(int rank, int *adjacent_rank) {
    int direction[4][2] = { {1,0}, {0, 1}, {-1, 0}, {0, -1}};
    for (int i = 0; i < 4; i++) {
        int next_rank = rank + direction[i][0] * col + direction[i][1];
        if (next_rank < 0 || next_rank >= Base_station_rank) adjacent_rank[i] = -1;
        else adjacent_rank[i] = next_rank;
    }
}

void BStation::init_nodes_avail()
{
    nodes_avail.resize(Base_station_rank);
    std::fill(nodes_avail.begin(), nodes_avail.end(), true);
}

int format_to_datetime(time_t t, char* out_buf, size_t out_buf_len) {
    struct tm* tm = localtime(&t);
    return strftime(out_buf, out_buf_len, "%c", tm);
}

/**
 * log the message in Base station
 * and send available nearby Nodes to report EVnode
*/
void BStation::process_alert_report() {
    int nearby_avail_nodes[Base_station_rank];
    int num_of_avail = 0;

    while (true) {
        if (!alert_msgs.empty()) {
            BS_log alert = alert_msgs.front();
            alert_msgs.pop_front();
            EVNodeMessage* msg = alert.msg;
            time_t recv_time = alert.log_t;
            int cur_iter = alert.log_iteration;
            num_of_avail = 0;

            get_available_EVNodes(msg, nearby_avail_nodes, &num_of_avail);

            MPI_Send(&nearby_avail_nodes[0] , 1 , MPI_INT , msg->rank , NEARBY_AVAIL_MESSAGE , MPI_COMM_WORLD);

            do_alert_log(msg, recv_time, nearby_avail_nodes, num_of_avail, cur_iter);
        }
    }
};

void BStation::print_log(std::string info) {
    if (log_fp) {
        fprintf(log_fp, "%s\n", info.c_str());
    }
}

void BStation::do_alert_log(EVNodeMessage* msg, time_t log_time, int *nearby_avail_nodes, int num_of_avail, int cur_iter) {
    std::string divider = "------------------------------------------------------------------------------------------------------------\n";
    print_log(divider);

    std::string info = "Iteration : " + std::to_string(cur_iter) + "\n";
    print_log(info);

    std::string log_t = ctime(&log_time);
    info = "Logged time : \t\t\t" + log_t + "\n";

    std::string report_node = "Reporting Node : " + std::to_string(msg->rank) + "\n";
    print_log(report_node);

    std::string neighbor_node;
    for (int i = 0; i < 4; i++) {
        if (msg->neighbor_ranks[i] != MPI_PROC_NULL) {
            neighbor_node = "Adjacent Nodes : " + std::to_string(msg->neighbor_ranks[i]) + "\n";
            print_log(neighbor_node);
        }
    } 

    std::string nearby_avail;
    for (int i = 0; i < num_of_avail; i++) {
        nearby_avail = "Nearby Nodes : " + std::to_string(nearby_avail_nodes[i]) + "\n";
        print_log(nearby_avail);
    }

    print_log(divider);
}
