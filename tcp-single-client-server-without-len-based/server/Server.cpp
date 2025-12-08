#include "Server.hpp"

Server::Tcp::Tcp() : _sockFd(0), _port(32999), _clientSockFd(0), _msglen(2000)
{
    memset(&_sockAddr, '\0', sizeof(_sockAddr));
    memset(&_clientSockAddr, '\0', sizeof(_clientSockAddr));
    _clientSockSize = sizeof(sockaddr);
}

Server::Tcp::Tcp(uint16_t p_port) : _sockFd(0), _port(p_port), _clientSockFd(0), _msglen(2000)
{
    memset(&_sockAddr, '\0', sizeof(_sockAddr));
    memset(&_clientSockAddr, '\0', sizeof(_clientSockAddr));
    _clientSockSize = sizeof(sockaddr);
}

void Server::Tcp::prepareAndWaitForClient()
{
    // Create a socket with address family , Socket type and protocol

    _sockFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (_sockFd < 0)
    {
        std::cerr << "Socket Creation failed \n";
        return;
    }

    // Initialize a address structure

    _sockAddr.sin_family = AF_INET;
    _sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    _sockAddr.sin_port = htons(_port);
    memset(_sockAddr.sin_zero, 0, sizeof(_sockAddr.sin_zero));

    int yes = 1;
    setsockopt(_sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // bind the initialized address to the created socket

    if (bind(_sockFd, (struct sockaddr *)&_sockAddr, sizeof(_sockAddr)) < 0)
    {
        std::cerr << "Binding the socket failed \n";
        return;
    }

    // listen the incoming client

    if (listen(_sockFd, 10) < 0)
    {
        std::cerr << "Listening Failed \n";
        return;
    }
}

void Server::Tcp::acceptClient()
{
    // accept the incoming client

    _clientSockFd = accept(_sockFd, (struct sockaddr *)&_clientSockAddr, &_clientSockSize);
}

void Server::Tcp::sendMsg()
{
    std::string frameMsg{"Hi this is Server got msg . Roger that!!"};

    send(_clientSockFd, frameMsg.c_str(), frameMsg.size(), 0);
}

void Server::Tcp::recvMsg()
{
    char *tempBuff = new char[2000];
    recv(_clientSockFd, tempBuff, 2000, 0);

    std::cout << "Message : " << tempBuff << std::endl;
}

int main()
{

    Server::Tcp tcpServerInst1;
    tcpServerInst1.prepareAndWaitForClient();
    tcpServerInst1.acceptClient();
    tcpServerInst1.recvMsg();
    tcpServerInst1.sendMsg();
    return 0;
}
