#include "Server.hpp"

pollServer::Tcp::Tcp(uint16_t p_port) : _port(p_port), _maxFd(0), _masterSockFd(-1), _clientFd(-1)
{
    memset(&_sockAddr, 0, sizeof(_sockAddr));
    memset(&_clientSockAddr, 0, sizeof(_clientSockAddr));
    _sockLen = sizeof(_clientSockAddr);
    _sockBook.clear();
}

void pollServer::Tcp::setNonBlockingSocket(int& p_sock)
{
    int flags = fcntl(p_sock, F_GETFL, 0);
    (flags == -1) ? flags = 0 : flags;
    fcntl(p_sock, F_SETFL, flags | O_NONBLOCK);
}

void pollServer::Tcp::initAndListenForIncomingconn()
{
    // Create a socket

    _masterSockFd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (_masterSockFd < 0)
    {
        std::cerr << "Socket creation failed \n";
        return;
    }

    // Initialize socket address

    _sockAddr.sin_family = AF_INET;
    _sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    _sockAddr.sin_port = htons(_port);
    memset(&_sockAddr.sin_zero, 0, sizeof(_sockAddr.sin_zero));

    // Set socket property
    int opt = 1;
    if (::setsockopt(_masterSockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "Setting socket properties failed \n";
        return;
    }

    setNonBlockingSocket((int&)_masterSockFd);

    // Bind socket to address

    if (::bind(_masterSockFd, (sockaddr *)&_sockAddr, sizeof(_sockAddr)) < 0)
    {
        std::cerr << "Binding socket failed \n";
        return;
    }

    if ((::listen(_masterSockFd, 10)) < 0)
    {
        std::cerr << "Listening to the clients failed\n";
        return;
    }
    _sockBook.emplace_back(pollfd{_masterSockFd,POLLIN,0});
    _maxFd = 1;
}

bool pollServer::Tcp::sendMessage(int fd, const std::string &msg)
{
    uint32_t netLen = htonl(static_cast<uint32_t>(msg.size()));
    // MSG_NOSIGNAL avoids SIGPIPE on Linux
    ssize_t n1 = ::send(fd, &netLen, sizeof(netLen), MSG_NOSIGNAL);
    if ((n1 != static_cast<ssize_t>(sizeof(netLen))) || (n1<0))
        return false;

    ssize_t n2 = ::send(fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
    return (n2 == static_cast<ssize_t>(msg.size()) && (n2 > 0));
}

ssize_t pollServer::Tcp::recv_all(int fd, void *buf, size_t len)
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

bool pollServer::Tcp::receiveAndProcess(int fd)
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
    if (!sendMessage(fd, msg))
    {
        std::cerr << "Failed to send ACK to fd=" << fd << std::endl;
        return false;
    }
    return true;
}

void pollServer::Tcp::pollAndAcceptClients()
{
    while (true)
    {
        int activity = poll(_sockBook.data(), _maxFd, -1);

        if (activity < 0)
        {
            std::cerr << "poll error \n";
            break;
        }

        for (uint64_t fdSearch = 0; fdSearch < _maxFd; fdSearch++)
        {
            if (_sockBook[fdSearch].fd < 0)
                continue;

            // Handle disconnects/errors first
            if (_sockBook[fdSearch].revents & (POLLHUP | POLLERR))
            {
                close(_sockBook[fdSearch].fd);
                _sockBook[fdSearch].fd = -1;
                continue;
            }

            if (_sockBook[fdSearch].revents & POLLIN)
            {
                if (_sockBook[fdSearch].fd == _masterSockFd)
                {
                    _clientFd = accept(_masterSockFd, (sockaddr *)&_clientSockAddr, &_sockLen);

                    if (_clientFd)
                    {
                        // Make new connections in non blocking mode

                        setNonBlockingSocket((int&)_clientFd);
                        _sockBook.push_back(pollfd{_clientFd,POLLIN,0});
                        // Set maxFd
                        _maxFd++;
                        std::cout << "New client connected: " << _clientFd << std::endl;
                    }
                    else
                    {
                        close(_sockBook[fdSearch].fd);
                    }
                }
                else
                {

                    if (!receiveAndProcess(_sockBook[fdSearch].fd))
                    {
                        std::cout << "Closing connection on fd " << _sockBook[fdSearch].fd << std::endl; 
                        close(_sockBook[fdSearch].fd); 
                        _sockBook.erase(_sockBook.begin() + fdSearch); 
                        _maxFd = _sockBook.size(); 
                        fdSearch--; // adjust index
                    }
                }
            }
        }
    }
}

int main()
{
    pollServer::Tcp Server;
    Server.initAndListenForIncomingconn();
    Server.pollAndAcceptClients();
    return 0;
}
