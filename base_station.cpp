#include <cstddef>
#include <sys/time.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <utility>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include <linux/if.h>

#include "base_station.h"

BStation::BStation(unsigned int _iteration_interval, unsigned int _iterations_num, int _row, int _col) : 
    iteration_interval(_iteration_interval), iterations_num(_iterations_num), row(_row), col(_col) {
    
    cur_iteration = 0;
    Base_station_rank = row * col;
    alert_events = 0;
    iprobe_count = 0;

    log_fp = fopen(LOG_FILE, "a");
    if (log_fp == NULL) {
        printf( "Could not open file %s\n", LOG_FILE);
        exit(-1);
    }
    
    MPIHelper::create_EV_message_type(&EV_msg_type);
    std::thread listen_thread(&BStation::listen_report_from_WSN, this, &alert_events);
    std::thread timer_thread(&BStation::iteration_recorder, this);
    std::thread process_alert_thread(&BStation::process_alert_report, this);

    listen_thread.join();
    process_alert_thread.join();
    timer_thread.join();
}

BStation::~BStation() {
    if (log_fp != NULL) fclose(log_fp);
}

void BStation::iteration_recorder() {
    while (cur_iteration < iterations_num) {
        sleep(iteration_interval);
        
        cur_iteration++;
    }

    // send ternimate signal to all EVNode
    // begin send terminate until listen thread and alert thread end
    while (!alert_msgs.empty()) ;
    send_ternimate_signal();

    print_log("******************* Summary ******************* \nnumber of reported messages : " + std::to_string(alert_events) + \
            ",and a summary of events generated : " + std::to_string(alert_events));
    print_log("MPI_Send counts : " + std::to_string(alert_events + Base_station_rank));
    print_log("MPI_Recv counts : " + std::to_string(alert_events));
    print_log("MPI_Iprobe counts : " + std::to_string(iprobe_count));
    print_log("*********************************************** \n");

    MPI_Type_free(&EV_msg_type);
}

void BStation::listen_report_from_WSN(int *alert_events) {
    timeval recv_time;
    EVNodeMessage msg;
    MPI_Status stat;
    MPI_Status probe_stat;
    int flag = 0;

    while (cur_iteration < iterations_num) {
        // check if a EVNode has sent an alert message
        MPI_Iprobe(MPI_ANY_SOURCE, ALERT_MESSAGE, MPI_COMM_WORLD, &flag, &probe_stat);
        ++iprobe_count;
        while (flag) {
            MPI_Recv(&msg, 1, EV_msg_type, probe_stat.MPI_SOURCE, ALERT_MESSAGE, MPI_COMM_WORLD, &stat);
            gettimeofday(&recv_time, NULL);    
            BS_log alert_log;
            // printf("recv from rank : %d\n", msg.rank);
            alert_log.msg = msg;
            alert_log.log_t = recv_time;
            alert_log.log_iteration = cur_iteration;
            alert_msgs.push(alert_log);
            (*alert_events)++;
            // print_log("recv alert report from : " + std::to_string(msg.rank));
            // std::fflush(log_fp);
            send_alert.emplace(((long long)msg.rank << 32l) | alert_log.log_iteration, true);
            // keep checking if more messages available and avoid losing message when cur_iteration >= iterations_num
            
            sleep(iteration_interval/10);
            MPI_Iprobe(MPI_ANY_SOURCE, ALERT_MESSAGE, MPI_COMM_WORLD, &flag, &probe_stat);
            ++iprobe_count;
        }
        if (!flag) sleep(iteration_interval/10);
    }
}

/**
 * send ternimate signal to all EVnode
*/
void BStation::send_ternimate_signal() {
    char buf = '1';
    // send message to terminate
    printf("send terminate\n");

    for (int i = 0; i < Base_station_rank; i++) {
        MPI_Send(&buf, 1, MPI_CHAR, i, TERMINATE, MPI_COMM_WORLD);
    }
    
    printf("send terminate ok\n");
}


/**
 * get nearby available nodes
*/
void BStation::get_available_EVNodes(EVNodeMessage* msg, int *node_list, int *num_of_list, int cur_iter) {
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
void BStation::process_alert_report() {
    int num_of_avail = 0;
    printf("enter loop\n");
    fflush(stdout);

    while (cur_iteration < iterations_num || !alert_msgs.empty()) {
        if (!alert_msgs.empty()) {
            int nearby_avail_nodes[Base_station_rank] = {-1};
            BS_log alert;
            num_of_avail = 0;
            alert_msgs.pop(alert);

            get_available_EVNodes(&(alert.msg), nearby_avail_nodes, &num_of_avail, alert.log_iteration);
            // std::string start_alert_process = "Base Station start alert process : " + std::to_string(Base_station_rank);
            // print_log(start_alert_process);
            // std::fflush(log_fp);

            // printf("before sending nearby , avail_nodes : %d\tdest rank : %d\n", nearby_avail_nodes[0], alert.msg.rank);
            MPI_Send(&nearby_avail_nodes[0] , 1 , MPI_INT, alert.msg.rank, NEARBY_AVAIL_MESSAGE, MPI_COMM_WORLD);

            do_alert_log(&(alert.msg), alert.log_t, nearby_avail_nodes, num_of_avail, alert.log_iteration);
        }
    }
    
    printf("exit process alert loop \n");
};


bool BStation::check_last_3iter(int rank, int iter) {
    if (send_alert.count((((long long)rank << 32l) | iter)) || 
        send_alert.count((((long long)rank << 32l) | (iter - 1))) ||
        send_alert.count((((long long)rank << 32l) | (iter - 2)))
        ) {
            return false;
        }

    else return true;
}

void BStation::print_log(std::string info) {
    if (log_fp) {
        fprintf(log_fp, "%s\n", info.c_str());
    }
}

void get_device_addresses(unsigned char ip_addr[4]) {
    struct ifaddrs *ifaddr, *ifa;
    struct sockaddr_in *ip_a;
    unsigned char *tmp_ip_addr;

    // get linked list of network interfaces
    if (getifaddrs(&ifaddr) == -1) return;

    // traverse linked list
    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET) {
            ip_a = (struct sockaddr_in *)ifa->ifa_addr;
            tmp_ip_addr = (unsigned char *)&ip_a->sin_addr.s_addr;
            memcpy(ip_addr, tmp_ip_addr, 4 * sizeof(unsigned char));
        }
    }
    freeifaddrs(ifaddr);
}

void format_ip_addr(unsigned char ip_addr[4], std::string& out_str) {
    out_str = std::to_string((int)ip_addr[0]) + "." + std::to_string((int)ip_addr[1]) + "." + std::to_string((int)ip_addr[2]) + "." + std::to_string((int)ip_addr[3]);
}

void BStation::do_alert_log(EVNodeMessage* msg, timeval recv_time, int *nearby_avail_nodes, int num_of_avail, int cur_iter) {
    std::string divider = "------------------------------------------------------------------------------------------------------------";
    print_log(divider);

    std::string info = "Iteration : " + std::to_string(cur_iter);
    print_log(info);

    time_t log_t = time(nullptr);
    std::string time_log = ctime(&log_t);
    time_log.pop_back();
    info = "Logged time : \t\t\t\t\t" + time_log;
    print_log(info);

    std::string alert_t = ctime(&(msg->alert_time));
    alert_t.pop_back();
    info = "Alert reported time : \t\t\t" + alert_t;
    print_log(info);

    info = "Number of adjacent node : " + std::to_string(msg->matching_neighbours);
    print_log(info);

    info = "Availability to be considered full : " + std::to_string(consider_full) + "\n";
    print_log(info);

    unsigned char ip_addr[4] = {0,0,0,0};
    info = "Reporting Node \t Coord \t\t Port Value \t Available Port \t IPv4";
    print_log(info);

    get_device_addresses(ip_addr);
    std::string ip;
    format_ip_addr(ip_addr, ip);

    info = std::to_string(msg->rank) + "\t\t\t\t (" + std::to_string(msg->rank / col) + "," + std::to_string(msg->rank % col) + ")" \
            + "\t\t 5\t\t\t\t " + std::to_string(msg->avail_ports) + "\t\t\t\t\t " + ip + "\n";
    print_log(info);

    info = "Adjacent Nodes \t Coord \t\t Port Value \t Available Port \t IPv4";
    print_log(info);

    std::string neighbor_node;
    for (int i = 0; i < 4; i++) {
        if (msg->neighbor_ranks[i] != MPI_PROC_NULL) {
            neighbor_node = std::to_string(msg->neighbor_ranks[i]) + "\t\t\t\t (" + std::to_string(msg->neighbor_ranks[i] / col) + "," + \
            std::to_string(msg->neighbor_ranks[i] % col) + ")" + "\t\t 5\t\t\t\t " + std::to_string(msg->neighbor_avail_ports[i]) + "\t\t\t\t\t " + ip;
            print_log(neighbor_node);
        }
    } 
    
    print_log("");

    info = "Nearby Nodes \t Coord \t";
    print_log(info);

    std::string nearby_avail;
    for (int i = 0; i < num_of_avail; i++) {
        nearby_avail = std::to_string(nearby_avail_nodes[i]) + "\t\t\t\t (" + std::to_string(nearby_avail_nodes[i] / col) + "," + std::to_string(nearby_avail_nodes[i] % col) + ")";
        print_log(nearby_avail);
    }

    print_log("");


    info = "Available station nearby (no report received in last 3 iteration) : ";
    for (int i = 0; i < num_of_avail; i++) {
        if (check_last_3iter(nearby_avail_nodes[i], cur_iter)) info += std::to_string(nearby_avail_nodes[i]) + ",";
    }
    info.pop_back();
    print_log(info);

    print_log("Total Messages send between reporting node and base station: 2");

    print_log(divider);
    std::fflush(log_fp);
}

