#include <cstddef>
#include <sys/time.h>
#include <thread>
#include <time.h>
#include <unistd.h>

#include "base_station.h"

BStation::BStation(unsigned int iteration_interval_, unsigned int total_iter_, int row_, int col_): 
    row(row_), col(col_), iteration_interval(iteration_interval_), total_iter(total_iter_),
    now_iter(0), bs_rank(row_ * col_), alert_events(0), logger(BS_LOG_FILE)
{    
    MPIHelper::create_EV_message_type(&EV_msg_type);
    std::thread listen_thread(&BStation::listen_alert, this, &alert_events);
    std::thread timer_thread(&BStation::iteration_timer, this);

    listen_thread.join();
    timer_thread.join();
}

void BStation::iteration_timer() {
    while (now_iter < total_iter) {
        sleep(iteration_interval);
        now_iter++;
    }

    // send ternimate signal to all EVNode
    send_ternimate();

    MPI_Type_free(&EV_msg_type);
}

void BStation::listen_alert(int *alert_events) {
    timeval recv_time;
    EVNodeMesg msg;
    MPI_Status stat;
    MPI_Status probe_stat;
    int flag = 0;

    while (now_iter < total_iter) {
        // check if receive an alert message
        MPI_Iprobe(MPI_ANY_SOURCE, ALERT_MESSAGE, MPI_COMM_WORLD, &flag, &probe_stat);

        if (flag) {
            MPI_Recv(&msg, 1, EV_msg_type, probe_stat.MPI_SOURCE, ALERT_MESSAGE, MPI_COMM_WORLD, &stat);
            gettimeofday(&recv_time, NULL);
            BS_log alert_log;
            alert_log.msg = msg;
            alert_log.log_t = recv_time;
            alert_log.log_iteration = now_iter;
            process_alert(alert_log);
            (*alert_events)++;
        }
    }
}

/**
 * send ternimate signal to all EVnode
*/
void BStation::send_ternimate() {
    char buf = '1';

    // send message to terminate
    for (int i = 0; i < bs_rank; i++) {
        MPI_Send(&buf, 1, MPI_CHAR, i, TERMINATE_MESSAGE, MPI_COMM_WORLD);
    }
}

/**
 * get nearby available nodes
*/
void BStation::get_available_EVNodes(EVNodeMesg* msg, int *node_list, int *num_of_list, int cur_iter) {
    std::set<int> nearby_node;
    for (int i = 0; i < 4; ++i) {
        int neighbor_rank = msg->neighbor_ranks[i];
        if (neighbor_rank != MPI_PROC_NULL) {
            // the adjacent nodes of the neighbouring nodes
            int adjacent_rank[4];
            get_neighbor_rank(neighbor_rank, adjacent_rank);
            // printf("%d %d %d %d\n", adjacent_rank[0], adjacent_rank[1], adjacent_rank[2], adjacent_rank[3]);
            for (int j = 0; j < 4; j++) {
                if (adjacent_rank[j] == -1 || send_alert.count((((long long)adjacent_rank[j] << 32l) | cur_iter))) continue;

                nearby_node.insert(adjacent_rank[j]);
            }
        }
    }

    for (int nearby_rank : nearby_node) {
        if (nearby_rank == msg->rank) continue;
        node_list[(*num_of_list)++] = nearby_rank;
    }
}


void BStation::get_neighbor_rank(int rank, int *adjacent_rank) {
    int r = rank / col, c = rank % col;
    int direction[4][2] = { {1,0}, {0, 1}, {-1, 0}, {0, -1}};
    for (int i = 0; i < 4; i++) {
        int nr = r + direction[i][0], nc = c + direction[i][1];
        if (nr < 0 || nr >= row || nc < 0 || nc >= col) adjacent_rank[i] = -1;
        else adjacent_rank[i] = nr * col + nc;
    }
}


/**
 * log the message in Base station
 * and send available nearby Nodes to report EVnode
*/
void BStation::process_alert(BS_log &alert) {
    int num_of_avail = 0;
    int nearby_avail_nodes[bs_rank] = {-1};

    get_available_EVNodes(&(alert.msg), nearby_avail_nodes, &num_of_avail, alert.log_iteration);

    MPI_Send(&nearby_avail_nodes[0] , 1 , MPI_INT, alert.msg.rank, NEARBY_MESSAGE, MPI_COMM_WORLD);

    do_alert_log(&(alert.msg), alert.log_t, nearby_avail_nodes, num_of_avail, alert.log_iteration);

    send_alert.emplace(((long long)alert.msg.rank << 32l) | alert.log_iteration, true);
};


bool BStation::check_evnode_avail(int rank, int iter) {
    if (send_alert.count((((long long)rank << 32l) | iter)) || 
        send_alert.count((((long long)rank << 32l) | (iter - 1))) ||
        send_alert.count((((long long)rank << 32l) | (iter - 2)))) 
    {
        return false;
    }
    return true;
}

void BStation::do_alert_log(EVNodeMesg* msg, timeval recv_time, int *nearby_avail_nodes, int num_of_avail, int cur_iter) {
    std::string divider(50, '-');
    logger.writeback_log(divider);

    std::string info = "Iteration : " + std::to_string(cur_iter);
    logger.writeback_log(info);

    time_t log_t = time(nullptr);
    std::string time_log = ctime(&log_t);
    time_log.pop_back();
    info = "Logged time : \t\t\t\t\t" + time_log;
    logger.writeback_log(info);

    std::string alert_t = ctime(&(msg->alert_time));
    alert_t.pop_back();
    info = "Alert reported time : \t\t\t" + alert_t;
    logger.writeback_log(info);

    info = "Number of adjacent node : " + std::to_string(msg->neighbor_num);
    logger.writeback_log(info);

    info = "Availability to be considered full : " + std::to_string(full_threshold) + "\n";
    logger.writeback_log(info);

    unsigned char ip_addr[4] = {0,0,0,0};
    info = "Reporting Node \t Coord \t\t Port Value \t Available Port \t IPv4";
    logger.writeback_log(info);

    MPIHelper::get_device_addresses(ip_addr);
    std::string ip;
    MPIHelper::format_ip_addr(ip_addr, ip);

    info = std::to_string(msg->rank) + "\t\t\t\t (" + std::to_string(msg->rank / col) + "," + std::to_string(msg->rank % col) + ")" \
            + "\t\t 5\t\t\t\t " + std::to_string(msg->avail_ports) + "\t\t\t\t\t " + ip + "\n";
    logger.writeback_log(info);

    info = "Adjacent Nodes \t Coord \t\t Port Value \t Available Port \t IPv4";
    logger.writeback_log(info);

    std::string neighbor_node;
    for (int i = 0; i < 4; i++) {
        if (msg->neighbor_ranks[i] != MPI_PROC_NULL) {
            neighbor_node = std::to_string(msg->neighbor_ranks[i]) + "\t\t\t\t (" + std::to_string(msg->neighbor_ranks[i] / col) + "," + \
            std::to_string(msg->neighbor_ranks[i] % col) + ")" + "\t\t 5\t\t\t\t " + std::to_string(msg->neighbor_availability[i]) + "\t\t\t\t\t " + ip;
            logger.writeback_log(neighbor_node);
        }
    } 
    
    logger.writeback_log("");

    info = "Nearby Nodes \t Coord \t";
    logger.writeback_log(info);

    std::string nearby_avail;
    for (int i = 0; i < num_of_avail; i++) {
        nearby_avail = std::to_string(nearby_avail_nodes[i]) + "\t\t\t\t (" + std::to_string(nearby_avail_nodes[i] / col) + "," + std::to_string(nearby_avail_nodes[i] % col) + ")";
        logger.writeback_log(nearby_avail);
    }

    logger.writeback_log("");


    info = "Available station nearby: ";
    for (int i = 0; i < num_of_avail; i++) {
        if (check_evnode_avail(nearby_avail_nodes[i], cur_iter)) info += std::to_string(nearby_avail_nodes[i]) + ",";
    }
    info.pop_back();
    logger.writeback_log(info);

    logger.writeback_log("Total Messages send between reporting node and base station: 2");

    logger.writeback_log(divider);
    logger.flush_log();
}

