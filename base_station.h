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
    BStation(int _listen_interval, int _terminate);
    ~BStation();

private:
    int Base_station_rank;
    FILE* log_fp;
    int terminate;
    unsigned int listen_interval;

    const char* LOG_FILE = "base_station.log";


    void process_EVNode_message(EVNodeMessage* msg);
    void listen_report_from_WSN();
    void send_terminal_signal();
};



#endif