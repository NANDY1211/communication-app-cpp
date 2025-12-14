#include "Server.hpp"

Server::Tcp::Tcp(uint16_t p_port = 32999) : _port(p_port), _masterSock(0), _clientSockFd(0), _maxFd(0), _msgLen(0)
{
    memset(&_sockAddr, 0, sizeof(_sockAddr));
    memset(&_clientSockAddr, 0, sizeof(_clientSockAddr));
    _clientSockLen = sizeof(_clientSockAddr);
    FD_ZERO(&_masterFds);
    FD_ZERO(&_tempFds);
}

void Server::Tcp::prepareAndListenToclients()
{
    _masterSock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (_masterSock < 0)
    {
        std::cerr << "Failed to create socket \n";
        return;
    }

    int opt = 1;
    if (::setsockopt(_masterSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR)");
        ::close(_masterSock);
        _masterSock = -1;
        return;
    }
    _sockAddr.sin_family = AF_INET;
    _sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    _sockAddr.sin_port = htons(_port);
    memset(_sockAddr.sin_zero, 0, sizeof(_sockAddr.sin_zero));

    if (::bind(_masterSock, reinterpret_cast<sockaddr *>(&_sockAddr), sizeof(_sockAddr)) < 0)
    {
        perror("bind");
        ::close(_masterSock);
        _masterSock = -1;
        return;
    }

    if (::listen(_masterSock, 128) < 0)
    {
        perror("listen");
        ::close(_masterSock);
        _masterSock = -1;
        return;
    }
    FD_SET(_masterSock, &_masterFds);
    _maxFd = _masterSock;
}

bool Server::Tcp::sendMessage(int fd, const std::string &msg)
{
    uint32_t netLen = htonl(static_cast<uint32_t>(msg.size()));
    // MSG_NOSIGNAL avoids SIGPIPE on Linux
    ssize_t n1 = ::send(fd, &netLen, sizeof(netLen), MSG_NOSIGNAL);
    if (n1 != static_cast<ssize_t>(sizeof(netLen)))
        return false;

    ssize_t n2 = ::send(fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
    return n2 == static_cast<ssize_t>(msg.size());
}

ssize_t Server::Tcp::recv_all(int fd, void *buf, size_t len)
{
    char *p = static_cast<char *>(buf);
    size_t done = 0;
    while (done < len)
    {
        ssize_t n = ::recv(fd, p + done, len - done, 0);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0)
            return 0; // peer closed
        done += static_cast<size_t>(n);
    }
    return static_cast<ssize_t>(done);
}

void Server::Tcp::acceptClients()
{
    int _clientFd = ::accept(_masterSock,
                             reinterpret_cast<sockaddr *>(&_clientSockAddr),
                             &_clientSockLen);
    if (_clientFd < 0)
    {
        perror("accept");
        return;
    }
    FD_SET(_clientFd, &_masterFds);
    if (_clientFd > _maxFd)
        _maxFd = _clientFd;

    std::cout << "New client connected: fd=" << _clientFd << std::endl;
}

// Returns false on disconnect/error, true if message processed
bool Server::Tcp::receiveAndProcess(int fd)
{
    uint32_t netLen = 0;
    ssize_t r = recv_all(fd, &netLen, sizeof(netLen));
    if (r <= 0)
        return false;

    uint32_t len = ntohl(netLen);

    // Sanity cap to avoid absurd allocations (e.g., 8MB)
    const uint32_t kMaxMsg = 8u * 1024u * 1024u;
    if (len == 0 || len > kMaxMsg)
    {
        std::cerr << "Invalid message length " << len << " from fd=" << fd << std::endl;
        return false;
    }

    std::string msg(len, '\0');
    if (recv_all(fd, &msg[0], len) <= 0)
        return false;

    std::cout << "Message from fd=" << fd << ": " << msg << std::endl;

    // Example response: ACK to sender (or broadcast to all)
    const std::string ack = "Ack from Server";
    if (!sendMessage(fd, ack))
    {
        std::cerr << "Failed to send ACK to fd=" << fd << std::endl;
        return false;
    }
    return true;
}

void Server::Tcp::waitingAndAcceptingConnections()
{
    if (_masterSock < 0)
        return;
    while (true)
    {
        _tempFds = _masterFds;

        int sel = select(_maxFd + 1, &_tempFds, NULL, NULL, NULL);

        if (sel < 0)
        {
            if (errno == EINTR)
                continue; // interrupted by signal
            perror("select");
            continue;
        }

        for (int fdSearch = 0; fdSearch <= _maxFd; ++fdSearch)
        {
            if (!FD_ISSET(fdSearch, &_tempFds))
                continue;

            if (fdSearch == _masterSock) // New client incoming to Server FD
            {
                acceptClients();
            }
            else
            {
                if(!receiveAndProcess(fdSearch))
                {
                    closeConnection(fdSearch);
                }
            }
        }
    }
}

void Server::Tcp::closeConnection(int fd)
{
    FD_CLR(fd, &_masterFds);
    ::close(fd);
    // Optionally shrink _maxFd if we just closed the highest fd
    if (fd == _maxFd)
    {
        for (int i = _maxFd - 1; i >= 0; --i)
        {
            if (FD_ISSET(i, &_masterFds))
            {
                _maxFd = i;
                break;
            }
        }
    }
    std::cout << "Closed fd=" << fd << std::endl;
}
int main()
{
    Server::Tcp l_instacne1;
    l_instacne1.prepareAndListenToclients();
    l_instacne1.waitingAndAcceptingConnections();

    return 0;
}