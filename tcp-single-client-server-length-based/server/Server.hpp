#include<cstdint>
#include<arpa/inet.h>
#include<string>
#include<iostream>
#include<unistd.h>
#include<string>
#include<cstring>


namespace Server
{
    class Tcp
    {
        private:
            uint16_t _sockFd;
            u_int16_t _clientSock;
            sockaddr_in _sockAddr;
            sockaddr_in _clientSockAddr;
            socklen_t _clientSockSize;
            uint32_t _msgLen;
            uint16_t _port;

        public:
            Tcp();
            explicit Tcp(uint16_t p_port);
            void prepareSocketAndWaitForClient();
            void acceptClient();
            void recvMsg();
            int recvMsgLength();
            void sendAckToClient();
            void closeConnection();
    };
}