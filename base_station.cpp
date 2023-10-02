#include <cstddef>
#include <cstdio>
#include <mpi.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <vector>

#include "base_station.h"
#include "types.h"
#include "wireless_sensor.h"
#include "mpi_helper.h"

BStation::BStation(unsigned int _iteration_interval, unsigned int _iterations_num, int _row, int _col) : 
    iteration_interval(_iteration_interval), iterations_num(_iterations_num), cur_iteration(0), row(_row), col(_col) {
    log_fp = fopen(LOG_FILE, "a");
    if (log_fp == NULL) {
        printf( "Could not open file %s\n", LOG_FILE);
        exit(-1);
    }

    MPIHelper::create_EV_message_type(&EV_msg_type);
    std::thread listen_thread(&BStation::listen_report_from_WSN, this);
    std::thread timer_thread(&BStation::iteration_recorder, this);

    listen_thread.join();
    timer_thread.join();
}

BStation::~BStation() {
    if (log_fp != NULL) fclose(log_fp);
    MPI_Type_free(&EV_msg_type);
}

/**
 * whole Processing flow of Base Station
*/
void BStation::BStation_run() {
    // listen report from WSN until the number of iterations has been reached
    int alert_events = 0;
    listen_report_from_WSN(&alert_events);

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
}

void BStation::iteration_recorder() {
    while (cur_iteration < iterations_num) {
        sleep(iteration_interval);
        
        for (auto alert : alert_msgs) {
            process_alert_report(alert.first, alert.second);
        }
        
        // if next iteration have get alert from that report node, that node should be available
        alert_msgs.clear();
        std::fill(nodes_avail.begin(), nodes_avail.end(), true);

        cur_iteration++;
    }
}

void BStation::listen_report_from_WSN(int *alert_events) {
    double start_time, recv_time;
    int messages_available;

    while (cur_iteration < iterations_num) {
        // check if a EVNode has sent an alert message
        EVNodeMessage msg;
        MPI_Iprobe(MPI_ANY_SOURCE, ALERT_MESSAGE, MPI_COMM_WORLD, &messages_available, MPI_STATUS_IGNORE);

        while (messages_available) {
            // recv and process EV node messages
            MPI_Recv(&msg, 1, EV_msg_type, MPI_ANY_SOURCE, ALERT_MESSAGE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            (*alert_events)++;
            
            recv_time = MPI_Wtime();
            alert_msgs.push_back({&msg, recv_time});

            // set as alert node
            nodes_avail[msg.rank] = false;
            // keep checking if more messages available
            MPI_Iprobe(MPI_ANY_SOURCE, ALERT_MESSAGE, MPI_COMM_WORLD, &messages_available, MPI_STATUS_IGNORE);
        }
    }
}

int BStation::cal_sleep_time(double start_time) {
    double end_time = MPI_Wtime();
    double sleep_length = (start_time + ((double)iteration_interval / 1000) - end_time) * 1e6;
    if ((long)sleep_length > 0) return (long)sleep_length;
    else return 0;
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
        if (next_rank < 0 || next_rank >= Base_station_rank) adjacent_coords[i][0] = adjacent_coords[i][0] = -1;
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
 * process aler msg from EV node and send available nearby Nodes to report EVnode
*/
void BStation::process_alert_report(EVNodeMessage* msg, double recv_time) {
    char log_msg[1024];
    char display_dt_logged[64];
    int len = 0;

    int nearby_avail_nodes[Base_station_rank];
    int num_of_avail = 0;
    get_available_EVNodes(msg, nearby_avail_nodes, &num_of_avail);

    MPI_Send( &nearby_avail_nodes[0] , 1 , MPI_INT , msg->rank , NEARBY_AVAIL_MESSAGE , MPI_COMM_WORLD);

    len += snprintf(log_msg + len, sizeof(log_msg) - len, "--------------------\n");

    // logged time

    format_to_datetime(time(NULL), display_dt_logged, sizeof(display_dt_logged));
    len += snprintf(log_msg + len, sizeof(log_msg) - len, "Logged time: %s\n",
                  display_dt_logged);

    // print details of neighbours to the reporting station
    for (int i = 0; i < msg->matching_neighbours; ++i) {
        len += snprintf(log_msg + len, sizeof(log_msg) - len, "%d\n", msg->neighbor_ranks[i]);
    }

    len += snprintf(log_msg + len, sizeof(log_msg) - len, "--------------------\n");
    printf("%s", log_msg);
    fprintf(log_fp, "%s", log_msg);
};