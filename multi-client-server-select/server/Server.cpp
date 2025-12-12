#include "Server.hpp"

Server::Tcp::Tcp() : _port(32999), _masterSock(0), _clientSockFd(0), _maxFd(0), _msgLen(0)
{
    memset(&_sockAddr, 0, sizeof(_sockAddr));
    memset(&_clientSockAddr, 0, sizeof(_clientSockAddr));
    _clientSockLen = sizeof(_clientSockAddr);
    FD_ZERO(&_masterFds);
    FD_ZERO(&_tempFds);
}

Server::Tcp::Tcp(uint16_t p_port) : _port(p_port), _masterSock(0), _clientSockFd(0), _maxFd(0), _msgLen(0)
{
    memset(&_sockAddr, 0, sizeof(_sockAddr));
    memset(&_clientSockAddr, 0, sizeof(_clientSockAddr));
    _clientSockLen = sizeof(_clientSockAddr);
    FD_ZERO(&_masterFds);
    FD_ZERO(&_tempFds);
}

void Server::Tcp::prepareAndListenToclients()
{
    _masterSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (_masterSock < 0)
    {
        std::cerr << "Failed to create socket \n";
        return;
    }

    int opt = 1;
    if (setsockopt(_masterSock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
    {
        std::cerr << "Failed to set socket options \n";
        return;
    }
    _sockAddr.sin_family = AF_INET;
    _sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    _sockAddr.sin_port = htons(_port);
    memset(_sockAddr.sin_zero, 0, sizeof(_sockAddr.sin_zero));

    if (bind(_masterSock, (struct sockaddr *)&_sockAddr, sizeof(_sockAddr)) < 0)
    {
        std::cerr << "Failed to bind \n";
        return;
    }

    FD_SET(_masterSock, &_masterFds);

    if (listen(_masterSock, 10) < 0)
    {
        std::cerr << "Listen failed \n";
        FD_CLR(_masterSock, &_masterFds);
        close(_masterSock);
        return;
    }

    FD_SET(_masterSock, &_masterFds);
    _maxFd = _masterSock;
}

void Server::Tcp::waitingAndAcceptingConnections()
{
    while (true)
    {
        _tempFds = _masterFds;

        int sel = select(_maxFd + 1, &_tempFds, NULL, NULL, NULL);

        if (sel < 0)
        {
            std::cerr << "Select Error \n";
            continue;
        }

        for (int fdSearch = 0; fdSearch <= _maxFd; fdSearch++)
        {
            if (FD_ISSET(fdSearch, &_tempFds))
            {

                if (fdSearch == _masterSock) // New client incoming to Server FD
                {
                    _tempFd = accept(_masterSock, (struct sockaddr *)&_clientSockAddr, &_clientSockLen);

                    if (_tempFd > 1024)
                    {
                        std::cerr << "Incoming client rejected\n";
                        continue;
                    }

                    FD_SET(_tempFd, &_masterFds);

                    if (_tempFd > _maxFd)
                        _maxFd = _tempFd;
                    std::cout << "New client connected: fd=" << _tempFd << std::endl;

                    recv_all(fdSearch, &_msgLen, sizeof(_msgLen));
                    std::string payload;
                    payload.resize(_msgLen);
                    recv_all(fdSearch, payload.data(), _msgLen);
                    sendAckToClient(fdSearch);
                }
                else
                {
                    recv_all(fdSearch, &_msgLen, sizeof(_msgLen));
                    std::string payload;
                    payload.resize(_msgLen);
                    recv_all(_tempFd, payload.data(), _msgLen);
                    sendAckToClient(fdSearch);
                }
            }
        }
    }
}

int Server::Tcp::receiveMsgLen()
{
    recv(_tempFd, &_msgLen, sizeof(_msgLen), 0);

    std::cout << "Expecting message of length: " << ntohl(_msgLen) << std::endl;

    return _msgLen;
}

void Server::Tcp::recvMsg()
{

    if (receiveMsgLen() < 0)
    {
        std::cerr << "Invalid message length \n";
        return;
    }
    char *userTempBuffer = new char[_msgLen + 1];

    recv(_tempFd, userTempBuffer, _msgLen, 0);

    std::cout << userTempBuffer << std::endl;
    delete[] userTempBuffer;
}

ssize_t Server::Tcp::recv_all(int fd, void *buf, size_t len)
{
    char *p = static_cast<char *>(buf);
    size_t done = 0;
    while (done < len)
    {
        ssize_t n = recv(fd, p + done, len - done, 0);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0)
        { // peer closed
            return 0;
        }
        done += static_cast<size_t>(n);
    }
    return static_cast<ssize_t>(done);
}

void Server::Tcp::sendAckToClient(uint16_t p_fd)
{
    std::string clientMsg{"Ack to Client from server"};
    uint32_t ackMsgLen = htonl(clientMsg.size());

    send(p_fd, &ackMsgLen, sizeof(ackMsgLen), 0);
    send(p_fd, clientMsg.c_str(), clientMsg.size(), 0);
}

void Server::Tcp::closeConnection()
{
    shutdown(_tempFd, SHUT_RDWR);
    close(_tempFd);
}

int main()
{
    Server::Tcp l_instacne1;
    l_instacne1.prepareAndListenToclients();
    l_instacne1.waitingAndAcceptingConnections();

    return 0;
}