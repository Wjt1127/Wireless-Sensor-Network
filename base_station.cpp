#include <mpi.h>
#include <thread>
#include <time.h>
#include <unistd.h>

#include "base_station.h"
#include "types.h"
#include "wireless_sensor.h"
#include "mpi_helper.h"

BStation::BStation(int _listen_interval, int _terminate) : listen_interval(_listen_interval), terminate(_terminate) {
    MPIHelper::create_EV_message_type(&EV_msg_type);
    std::thread listen_thread(&BStation::listen_report_from_WSN, this);
    listen_thread.join();
}

void BStation::listen_report_from_WSN() {
    while (1) {
        MPI_Status status;
        EVNodeMessage EV_msg;
        MPI_Request request;

        // recv message from any EVNode（MPI_ANY_SOURCE），
        // use Irecv(non-block)
        MPI_Irecv(&EV_msg, 1, EV_msg_type, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &request);

        int flag = 0;
        while (!flag) {
            MPI_Test(&request, &flag, &status);
            if (flag) {
                // recv message successfully and print message
                printf("Received message from rank %d: \n", status.MPI_SOURCE);
                BStation::process_EVNode_message(&EV_msg);
            } else {
                // no message wait for a period
                sleep(listen_interval);
            }
        }
    }
}

void BStation::send_terminal_signal(int dest) {
    char buf = '\0';
    MPI_Request bcast_req;
    // indicate to thread to terminate
    terminate = 1;
    // send message to terminate
    MPI_Send(&buf, 1, MPI_CHAR, dest, TERMINATE, MPI_COMM_WORLD);
}

int format_to_datetime(time_t t, char* out_buf, size_t out_buf_len) {
    struct tm* tm = localtime(&t);
    return strftime(out_buf, out_buf_len, "%c", tm);
}

void BStation::process_EVNode_message(EVNodeMessage* msg) {
    char log_msg[1024];
    int len = 0;

    len += snprintf(log_msg + len, sizeof(log_msg) - len, "--------------------\n");

    // logged time
    char display_dt_logged[64];
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