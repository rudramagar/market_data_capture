#ifndef SOCKET_H
#define SOCKET_H

#include <cstdint>
#include <string>

class Socket {
public:
    Socket();
    ~Socket();

    bool join_multicast_channel(const std::string& multicast_address, uint16_t multicast_port,
                        const std::string& interface, const std::string& source);

    // Set Kernel
    // receive buffer size
    // SO_RCVBUF
    bool set_recv_buffer(int size_bytes);

    // Receive one UDP packet
    // Returns bytes received, -1
    // on error
    int recv(uint8_t*, int max_buffer_size);

    // Get raw file
    // descriptor
    // for poll/eboll
    int fd() const;

    void close();

private:
    int sock_fd;
};

#endif
