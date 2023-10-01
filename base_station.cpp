#include "base_station.h"
#include "wireless_sensor.h"

int format_to_datetime(time_t t, char* out_buf, size_t out_buf_len) {
    struct tm* tm = localtime(&t);
    return strftime(out_buf, out_buf_len, "%c", tm);
}

void BStation::process_EVNode_message(EVNodeMessage* g_msg) {
    char log_msg[1024];
    int len = 0;

    len += snprintf(log_msg + len, sizeof(log_msg) - len, "--------------------\n");

    // logged time
    char display_dt_logged[64];
    format_to_datetime(time(NULL), display_dt_logged, sizeof(display_dt_logged));
    len += snprintf(log_msg + len, sizeof(log_msg) - len, "Logged time: %s\n",
                  display_dt_logged);

    // print details of neighbours to the reporting station
    for (int i = 0; i < g_msg->matching_neighbours; ++i) {
        len += snprintf(log_msg + len, sizeof(log_msg) - len, "%d\n", g_msg->neighbor_ranks[i]);
    }

    len += snprintf(log_msg + len, sizeof(log_msg) - len, "--------------------\n");
    printf("%s", log_msg);
    fprintf(log_fp, "%s", log_msg);
};