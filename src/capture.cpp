#include "capture.h"

#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <cstdio>
#include <cstring>
#include <ctime>

static uint64_t read_seq(const uint8_t* buf) {
    return ((uint64_t)buf[10] << 56) | ((uint64_t)buf[11] << 48) |
           ((uint64_t)buf[12] << 40) | ((uint64_t)buf[13] << 32) |
           ((uint64_t)buf[14] << 24) | ((uint64_t)buf[15] << 16) |
           ((uint64_t)buf[16] <<  8) | ((uint64_t)buf[17]);
}

static uint16_t read_msg_count(const uint8_t* buf) {
    return (buf[18] << 8) | buf[19];
}

bool Capture::start(const ServiceConfig& config, const std::string& output_path) {
    socket_count = 0;
    for (size_t i = 0; i < config.channels.size() && i < 2; i++) {
        const ChannelConfig& ch = config.channels[i];

        if (!sockets[i].join_multicast_channel(ch.multicast_address, ch.multicast_port,
                                       ch.interface, ch.source)) {
            std::fprintf(stderr, "ERROR: join failed %s:%u\n",
                         ch.multicast_address.c_str(), ch.multicast_port);
            syslog(LOG_ERR, "join failed %s:%u",
                   ch.multicast_address.c_str(), ch.multicast_port);
            return false;
        }

        sockets[i].set_recv_buffer(config.queue_size);
        socket_count++;
        std::fprintf(stderr, "JOINED: %s:%u\n",
                     ch.multicast_address.c_str(), ch.multicast_port);
    }

    if (socket_count == 0) {
        std::fprintf(stderr, "ERROR: no channels joined\n");
        return false;
    }

    // Open output file
    output_fd = ::open(output_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (output_fd < 0) {
        std::fprintf(stderr, "ERROR: cannot open %s\n", output_path.c_str());
        syslog(LOG_ERR, "cannot open %s", output_path.c_str());
        return false;
    }

    std::fprintf(stderr, "OUTPUT: %s\n", output_path.c_str());
    std::fprintf(stderr, "CAPTURE: running (Ctrl+C to stop)\n");

    // Setup poll
    struct pollfd poll_fds[2];
    for (int i = 0; i < socket_count; i++) {
        poll_fds[i].fd     = sockets[i].fd();
        poll_fds[i].events = POLLIN;
    }

    uint8_t recv_buffer[1500];
    time_t last_log_time       = ::time(nullptr);
    uint64_t last_log_received = 0;
    running = true;

    // Main loop: 
    // poll → recv → dedup → write
    while (running) {
        int ready = ::poll(poll_fds, socket_count, 100);

        time_t now = ::time(nullptr);
        if (now != last_log_time) {
            if (total_received != last_log_received) {
                syslog(LOG_INFO, "recv=%llu written=%llu dup=%llu gaps=%llu dropped=%llu",
                       (unsigned long long)total_received,
                       (unsigned long long)total_written,
                       (unsigned long long)total_dup,
                       (unsigned long long)total_gaps,
                       (unsigned long long)total_dropped);
                last_log_received = total_received;
            }
            last_log_time = now;
        }

        if (ready <= 0) {
            continue;
        }

        for (int i = 0; i < socket_count; i++) {
            if (!(poll_fds[i].revents & POLLIN)) {
                continue;
            }

            int bytes = sockets[i].recv(recv_buffer, sizeof(recv_buffer));
            if (bytes < 20) {
                continue;
            }

            // Skip heartbeats
            uint16_t msg_count = read_msg_count(recv_buffer);
            if (msg_count == 0) {
                continue;
            }

            // Sequence check
            uint64_t seq = read_seq(recv_buffer);

            if (first_packet) {
                expected_seq = seq;
                first_packet = false;
            }

            // Duplicate
            if (seq < expected_seq) {
                total_dup++;
                continue;
            }

            // Gap
            if (seq > expected_seq) {
                syslog(LOG_WARNING, "gap: expected=%llu got=%llu missing=%llu",
                       (unsigned long long)expected_seq,
                       (unsigned long long)seq,
                       (unsigned long long)(seq - expected_seq));
                total_gaps++;
            }

            expected_seq = seq + msg_count;
            total_received++;

            // Write: [4-byte length][raw packet]
            uint32_t packet_length = (uint32_t)bytes;
            if (::write(output_fd, &packet_length, sizeof(packet_length)) < 0 ||
                ::write(output_fd, recv_buffer, bytes) < 0) {
                total_dropped++;
                continue;
            }

            total_written++;
        }
    }

    // Flush and close
    ::fsync(output_fd);
    ::close(output_fd);
    output_fd = -1;

    for (int i = 0; i < socket_count; i++) {
        sockets[i].close();
    }

    syslog(LOG_INFO, "stopped: recv=%llu written=%llu dup=%llu gaps=%llu dropped=%llu",
           (unsigned long long)total_received,
           (unsigned long long)total_written,
           (unsigned long long)total_dup,
           (unsigned long long)total_gaps,
           (unsigned long long)total_dropped);

    return true;
}

void Capture::stop() {
    running = false;
}
