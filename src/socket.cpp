#include "socket.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

Socket::Socket() : sock_fd(-1) {}

Socket::~Socket() {
    close();
}

void Socket::close() {
    if (sock_fd >= 0) {
        ::close(sock_fd);
        sock_fd = -1;
    }
}

int Socket::fd() const {
    return sock_fd;
}

bool Socket::join_multicast_channel(const std::string& multicast_address, uint16_t multicast_port,
                            const std::string& interface, const std::string& source) {

    if (sock_fd >=0) {
        close();
    }

    sock_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        return false;
    }

    int reuse = 1;
    ::setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in bind_addr = {};
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(multicast_port);
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (::bind(sock_fd, (sockaddr*)&bind_addr, sizeof(bind_addr)) < 0) {
        close();
    }

    // SSM only
    ip_mreq_source multicast_rerequest = {};
    multicast_rerequest.imr_multiaddr.s_addr = ::inet_addr(multicast_address.c_str());
    multicast_rerequest.imr_interface.s_addr = ::inet_addr(interface.c_str());
    multicast_rerequest.imr_sourceaddr.s_addr = ::inet_addr(source.c_str());

    if (::setsockopt(sock_fd, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, &multicast_rerequest, sizeof(multicast_rerequest)) < 0) {
        close();
        return false;
    }

    return true;
}

bool Socket::set_recv_buffer(int size_bytes) {
    if (sock_fd < 0) {
        return false;
    }

    return ::setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &size_bytes, sizeof(size_bytes)) == 0;
}

int Socket::recv(uint8_t* buffer, int max_buffer_size) {
    if (sock_fd < 0) {
        return -1;
    }
    return (int)::recvfrom(sock_fd, buffer, (size_t)max_buffer_size, 0, nullptr, nullptr);
}
