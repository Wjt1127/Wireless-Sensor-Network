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

    ~BStation();

    void process_EVNode_message(EVNodeMessage* g_msg);

private:
    FILE* log_fp;   
    const char* LOG_FILE = "base_station.log";
};



#endif