#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>

namespace Client
{
    class Tcp
    {
    private:
        uint16_t _clientSockFd;
        uint16_t _port;
        sockaddr_in _clientSockAddr;
        size_t _msgLen;
        std::string _msg;

    public:
        explicit Tcp(uint16_t p_port);
        void prepareSocketAndConnectToServer();
        void recvMsg();
        static ssize_t recv_all(int fd, void *buf, size_t len);
        int recvMsgLength();
        void sendMsgToServer();
        bool sendMessage(const std::string &msg);
        bool receiveMessage(std::string &outMsg);
        void closeConnection();
    };
}
