#ifndef EV_LOGGER_H
#define EV_LOGGER_H

#include <mpi.h>
#include <string>

class MPILogger
{
public:

    MPILogger(std::string filename) {
        logfile = fopen(filename.c_str(), "a+");
    }

    ~MPILogger() {
        if (logfile) {
            fclose(logfile);
        }
    }

    void print_log(std::string info) {
        if (logfile) {
            fprintf(logfile, "%s\n", info.c_str());
            fflush(logfile);
        }
    }

    void writeback_log(std::string info) {
        if (logfile) {
            fprintf(logfile, "%s\n", info.c_str());
        }
    }

    void flush_log() {
        if (logfile) {
            fflush(logfile);
        }
    }

private:
    FILE* logfile;
};

#endif