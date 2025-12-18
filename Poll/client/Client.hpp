#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>
#include <algorithm>
#include <fcntl.h>
#include <poll.h>
#include <vector>

namespace Client
{
    class Tcp
    {
    private:
        uint16_t _clientSockFd;
        uint16_t _port;
        uint16_t _maxFd;
        sockaddr_in _clientSockAddr;
        size_t _msgLen;
        std::string _msg;
        std::vector<pollfd> _sockBook;

    public:
        explicit Tcp(uint16_t p_port = 32999);
        void prepareSocketAndConnectToServer();
        void recvMsg();
        static ssize_t recv_all(int fd, void *buf, size_t len);
        int recvMsgLength();
        bool sendMessage(const std::string &msg);
        bool receiveMessage(std::string &outMsg);
        void closeConnection();
        void chatLoop();
        void setNonBlockingSocket(int& p_sock);
    };
}
