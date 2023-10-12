#ifndef EV_LOGGER_H
#define EV_LOGGER_H

#include <mpi.h>
#include <string>

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

    void print_log(std::string info) {
        if (logfile) {
            fprintf(logfile, "%s\n", info.c_str());
            fflush(logfile);
        }
    }

private:
    FILE* logfile;
};

#endif