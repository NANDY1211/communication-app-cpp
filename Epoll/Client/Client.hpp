#pragma once

#include <string>
#include <vector>

#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

namespace epoll_client
{

    class TcpClient
    {
    public:
        TcpClient(uint16_t port = 17888);
        ~TcpClient();

        void connectToServer();
        void run();

    private:
        void setNonBlocking(int fd);
        void sendMessage(const std::string &msg);
        bool receiveMessage();

    private:
        std::string serverIp_;
        uint16_t serverPort_;

        int sockFd_;
        int epollFd_;

        std::vector<epoll_event> events_;
    };

}
