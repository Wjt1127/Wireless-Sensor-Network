#ifndef MPI_HELPER_H
#define MPI_HELPER_H

#include <utility>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include <linux/if.h>
#include <bits/types/time_t.h>
#include <mpi.h>
#include <mpi_proto.h>
#include <string>
#include <cstring>

typedef struct {
    int rank;
    int avail_ports;
    /* num of neighbor */
    int neighbor_num;
    /* rank of all neighbors */
    int neighbor_ranks[4];        
    /* 2-dim coordinations of neighbors */
    int neighbor_coords[4][2];
    /* the available port num of neighbor ports */
    int neighbor_availability[4];
    /* when the event occured */
    time_t alert_time;
} EVNodeMesg;

class MPIHelper
{
public:
    static void create_EV_message_type(MPI_Datatype *EV_message_type)
    {
        int num_of_fields = 7;
        int block_lengths[7] = {1, 1, 1, 4, 4 * 2, 4, 1};
        MPI_Aint displacements[7];
        displacements[0] = offsetof(EVNodeMesg, rank);
        displacements[1] = offsetof(EVNodeMesg, avail_ports);
        displacements[2] = offsetof(EVNodeMesg, neighbor_num);
        displacements[3] = offsetof(EVNodeMesg, neighbor_ranks);
        displacements[4] = offsetof(EVNodeMesg, neighbor_coords);
        displacements[5] = offsetof(EVNodeMesg, neighbor_availability);
        displacements[6] = offsetof(EVNodeMesg, alert_time);
        
        MPI_Datatype datatypes[7] = {
            MPI_INT,           MPI_INT,           MPI_INT,          MPI_INT,
            MPI_INT,           MPI_INT,           MPI_LONG
            };
        MPI_Type_create_struct(num_of_fields, block_lengths, displacements, datatypes, EV_message_type);
        MPI_Type_commit(EV_message_type);
    }

    static void get_device_addresses(unsigned char ip_addr[4]) {
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

    static void format_ip_addr(unsigned char ip_addr[4], std::string& out_str) {
        out_str = std::to_string((int)ip_addr[0]) + "." + std::to_string((int)ip_addr[1]) + "." + std::to_string((int)ip_addr[2]) + "." + std::to_string((int)ip_addr[3]);
    }
};

enum {
    SEND_ALERT_FAIL = 0,
    SEND_ALERT_SUCCESS = 1,
};

enum {
    AVAIL_MESSAGE = 1,
    ALERT_MESSAGE = 2,
    PROMPT_MESSAGE = 3,
    NEARBY_MESSAGE = 4,
    TERMINATE_MESSAGE = 5,
};

#endif