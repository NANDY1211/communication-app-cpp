#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace Server
{
    class Tcp
    {
        private:
            uint16_t _port;
            uint16_t _masterSock;
            uint16_t _clientSockFd;
            uint16_t _maxFd;
            uint16_t _clientFd;
            size_t _msgLen;
            sockaddr_in _sockAddr;
            sockaddr_in _clientSockAddr;
            socklen_t _clientSockLen;
            fd_set _masterFds;
            fd_set _tempFds;

        public:
            Tcp(uint16_t p_port);
            void prepareAndListenToclients();
            void waitingAndAcceptingConnections();
            static bool sendMessage(int clientFd, const std::string &msg);
            void closeConnection(int _fd);
            void recvMsg();
            static ssize_t recv_all(int fd, void *buf, size_t len); 
            void acceptClients();
            bool receiveAndProcess(int fd);

    };
}