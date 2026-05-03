#ifndef CAPTURE_H
#define CAPTURE_H

#include "config_parser.h"
#include "socket.h"
#include "sequence_tracker.h"

#include <cstdint>
#include <string>

class Capture {
public:
    bool start(const ServiceConfig& config, const std::string& output_path);
    void stop();

private:
    Socket   sockets[2];
    int      socket_count    = 0;
    int      output_fd       = -1;
    bool     running         = false;
    SequenceTracker sequence_tracker;
    uint64_t  highest_seq    = 0;
    uint64_t total_received  = 0;
    uint64_t total_written   = 0;
    uint64_t total_dropped   = 0;
    uint64_t total_dup       = 0;
    uint64_t total_gaps      = 0;
};

#endif
