#include <cstdint>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <string>
#include <cstring>

namespace Server
{
    class Tcp
    {
    private:
        uint16_t _sockFd;
        uint16_t _port;
        uint16_t _clientSockFd;
        sockaddr_in _sockAddr;
        sockaddr_in _clientSockAddr;
        socklen_t _clientSockSize;
        size_t _msglen;

    public:
        explicit Tcp(uint16_t p_port);
        void prepareAndWaitForClient();
        void acceptClient();
        void recvMsg();
        void sendMsg();
    };
}