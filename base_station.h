#ifndef BASE_STATION_H
#define BASE_STATION_H

#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "wireless_sensor.h"


class BStation {
public:
    BStation() = default;
    BStation(int _listen_interval);
    ~BStation();

private:
    unsigned int listen_interval;
    int Base_station_rank;
    FILE* log_fp;
    int terminate;
    MPI_Datatype EV_msg_type;
    const char* LOG_FILE = "base_station.log";

    void print_EVNode_message(EVNodeMessage* msg);
    void listen_report_from_WSN();
    void process_alert_report();
    void process_available_report();
    void send_terminal_signal(int dest_rank);
};



#endif