#ifndef WIRELESS_SENSOR_H
#define WIRELESS_SENSOR_H

#include <atomic>
#include <bits/types/FILE.h>
#include <bits/types/time_t.h>
#include <mpi.h>
#include <stdio.h>
#include <fstream>
#include <string>
#include <deque>
#include <vector>

#define MAX_DATA_LENGTH 1024
#define AVAILABILITY_TIME_CYCLE 10
#define FIXED_ARRAY_SIZE 512
#define LOG_PATH_PREFIX "./logs/evnode"

class EVLogger
{
public:

    EVLogger(std::string filename) {
        logfile = fopen(filename.c_str(), "a+");
    }

    ~EVLogger() {
        fclose(logfile);
    }

    void avail_log(int rank, std::string now, int avail) {
        now.pop_back();
        std::string info = "AVAIL_LOG: " + now + ", rank " + std::to_string(rank) + " availability is " + std::to_string(avail);
        print_log(info);
    }

    void prompt_log(int rank) {
        time_t t= time(nullptr);
        std::string now = ctime(&t);
        now.pop_back();
        std::string info = "PROMPT_LOG: " + now + ", rank " + std::to_string(rank) + " availability is 0 and starts to prompt";
        print_log(info);
    }

    void neighbor_log(int rank, int neighbor, int avail) {
        time_t t= time(nullptr);
        std::string now = ctime(&t);
        now.pop_back();
        std::string info = "NEIGHBOR_LOG: " + now + ", rank " + std::to_string(rank) + "\'s neighbor " + std::to_string(neighbor)
            + "\'s availability is " + std::to_string(avail);
        print_log(info);
    }

    void alert_log(int rank) {
        time_t t= time(nullptr);
        std::string now = ctime(&t);
        now.pop_back();
        std::string info = "ALERT_LOG: " + now + ", rank " + std::to_string(rank) + " starts to alert";
        print_log(info);
    }

    void nearby_log(int rank, int nearby_rank) {
        std::string info;
        time_t t= time(nullptr);
        std::string now = ctime(&t);
        now.pop_back();
        if (nearby_rank == MPI_PROC_NULL)
            info = "NEARBY_LOG: " + now + ", rank " + std::to_string(rank) + " has no available nearby EVnode";
        else 
            info = "NEARBY_LOG: " + now + ", rank " + std::to_string(rank) + " has available nearby EVnode " + std::to_string(nearby_rank);
        print_log(info);
    }

    void terminate_log(int rank) {
        time_t t= time(nullptr);
        std::string now = ctime(&t);
        now.pop_back();
        std::string info = "TERMINATE_LOG: " + now + ", rank " + std::to_string(rank) + " terminates";
        print_log(info);
    }

private:
    FILE* logfile;
    void print_log(std::string info) {
        if (logfile) {
            fprintf(logfile, "%s\n", info.c_str());
            fflush(logfile);
        }
    }
};


typedef struct {
    int rank;
    int matching_neighbours;  // size of neighbour(if the node at edge, matching_neighbours may be 2 or 3)
    int neighbor_ranks[4];  // rank of neighbours
    int neighbor_coords[4][2]; // 2-dims coordinations of neighbour
    int neighbor_avail_ports[4]; // the available port num of neighbor ports

    time_t alert_time;        // when the event occured
} EVNodeMessage;

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
    std::vector<int> ports_avail;
    std::deque<AvailabilityLog> avail_table;
    MPI_Comm EV_comm;
    MPI_Comm grid_comm;
    MPI_Datatype EV_msg_type;
    EVNodeMessage *msg;
    std::atomic_int full_log_num;
    std::atomic_int stop;
    EVLogger logger;
    
    void init_neighbors();
    void init_ports();

    void port_simulation(int port_id);
    void send_prompt();
    void send_alert(int base_station_rank);
    
    void receive_message();
    void proccess_prompt(int source);
    void proccess_neighbor_availability(int source, std::atomic_int *responsed);
    void process_terminate(int base_station_rank);
    void process_nearby(int base_station_rank);

    bool alert_or_not(EVNodeMessage* msg);

};

enum {
    AVAIL_LOG = 1,
    PROMPT_LOG = 2,
    NEIGHBOR_LOG = 3,
    ALERT_LOG = 4,
    NEARBY_LOG = 5,
    TERMINATE_LOG = 6,
};

#endif
