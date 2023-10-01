#ifndef WIRELESS_SENSOR_H
#define WIRELESS_SENSOR_H

#define MAX_DATA_LENGTH 1024
#include <mpi.h>
#include <stdio.h>
#include <fstream>
#include <string>
#include <deque>


#define MAX_DATA_LENGTH 1024
#define AVAILABILITY_TIME_CYCLE 10
#define FIXED_ARRAY_SIZE 512


typedef struct {
    int rank;
    int matching_neighbours;  // size of neighbour(if the node at edge, matching_neighbours may be 2 or 3)
    int neighbor_ranks[4];  // rank of neighbours
    int neighbor_coords[4][2]; // 2-dims coordinations of neighbour
    int neighbor_avail_ports[4]; // the available port num of neighbor ports

    double mpi_time;        // when the event occured
    
} EVNodeMessage;

struct TimeStamp
{
    unsigned short year;
    unsigned char mouth;
    unsigned char day;
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
};

struct AvailabilityLog
{
    TimeStamp time;
    unsigned int availability;
    friend std::istream& operator>>(std::istream &in, AvailabilityLog &log)
    {
        in >> log.time.year >> log.time.mouth >> log.time.day;
        in >> log.time.hour >> log.time.minute >> log.time.second;
        in >> log.availability;
        return in;
    }
};

class WirelessSensor
{
public:
    WirelessSensor() = default;
    WirelessSensor(int r_, int c_, int x_, int y_, std::string &source);

private:
    int row;
    int col;
    int x;
    int y;
    int ports_num;
    std::deque<AvailabilityLog> avail_table;

    void send_alert_to_base(int base_station_rank, char *alert_msg);
    void get_message_from_neighbor(MPI_Comm EV_Comm, EVNodeMessage *msg);
    void process_neighbor_message();
    void listen_terminal_from_base(int base_station_rank); 

    void report_availability(std::string avail_source);
    void prompt_availability();

    bool prompt_alert_or_not(EVNodeMessage* msg, int avail_neighbor[], int num_of_avail_neighbor);

};


#endif