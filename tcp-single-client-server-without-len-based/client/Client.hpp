#include <cstdint>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <string>
#include <cstring>


namespace Client
{
    class Tcp
    {
        private:
            uint16_t _port;
            uint16_t _clientSockFd;
            sockaddr_in _clientSockAddr;
            size_t _msgLen;
            std::string _msg;

        public:
            explicit Tcp(uint16_t p_port);
            void prepareAndConnectToServer();
            void sendMsg();
            void recvMsg();
    };
}