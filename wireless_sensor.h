#ifndef WIRELESS_SENSOR_H
#define WIRELESS_SENSOR_H

#define MAX_DATA_LENGTH 1024

typedef struct {
    int rank;
    int matching_neighbours;  // size of neighbour(if the node at edge, matching_neighbours may be 2 or 3)
    int neighbour_ranks[4]; // rank of neighbours

    double mpi_time;        // when the event occured
    unsigned char data[MAX_DATA_LENGTH];    // the message data
} EVNodeMessage;

class WirelessSensor
{
public:
    WirelessSensor() = default;
    WirelessSensor(int r_, int c_, int x_, int y_);
    int send_alert_to_base(char *alert_msg);
    void get_message_from_neighbor(int rank, char *msg);

private:
    int row;
    int col;
    int x;
    int y;
};


#endif