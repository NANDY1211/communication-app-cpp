#include <iostream> 
#include <string> 
#include <vector> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <poll.h> 
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>




namespace pollServer
{
    class Tcp
    {
        private:
            uint16_t _port;
            uint16_t _maxFd;
            uint16_t _masterSockFd;
            uint16_t _clientFd;
            sockaddr_in _sockAddr;
            sockaddr_in _clientSockAddr;
            socklen_t _sockLen;
            std::vector<pollfd> _sockBook;
            pollfd _clientPollFd;

        public:
            explicit Tcp(uint16_t p_port = 32999);
            void initAndListenForIncomingconn();
            void pollAndAcceptClients();
            static bool sendMessage(int clientFd, const std::string &msg);
            static ssize_t recv_all(int fd, void *buf, size_t len); 
            bool receiveAndProcess(int fd);
            void setNonBlockingSocket(int& p_sock);


    };
}