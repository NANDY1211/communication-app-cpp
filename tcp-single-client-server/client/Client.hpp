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
        Tcp();
        explicit Tcp(uint16_t p_port);
        void prepareSocketAndConnectToServer();
        void recvMsg();
        int recvMsgLength();
        void sendMsgToServer();
        void closeConnection();
    };
}
