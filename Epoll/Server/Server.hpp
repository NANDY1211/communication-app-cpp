#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>

#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

namespace epoll_server
{

    class Tcp
    {
    public:
        explicit Tcp(uint16_t port = 17888);
        virtual ~Tcp();

        void initSocketAndListen();
        void run();

    private:
        void setNonBlocking(int fd);
        void closeFd(int fd);
        void acceptConnections();
        bool receiveAndProcess(int fd);

    private:
        uint16_t port_;
        int masterFd_;
        int epollFd_;

        sockaddr_in serverAddr_{};
        sockaddr_in clientAddr_{};
        socklen_t clientLen_{};

        std::vector<epoll_event> events_;
    };

}
