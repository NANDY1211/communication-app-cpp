#include "Client.hpp"

Client::Tcp::Tcp(uint16_t p_port) : _clientSockFd(0), _port(p_port), _maxFd(0), _msgLen(0)
{
    memset(&_clientSockAddr, 0, sizeof(_clientSockAddr));
    _sockBook.reserve(2);
};

void Client::Tcp::setNonBlockingSocket(int &p_sock)
{
    int flags = fcntl(p_sock, F_GETFL, 0);
    if (flags == -1)
        flags = 0;
    fcntl(p_sock, F_SETFL, flags | O_NONBLOCK);
}

void Client::Tcp::prepareSocketAndConnectToServer()
{
    // Create a socket with address family , socket type and protocol
    _clientSockFd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (_clientSockFd < 0)
    {
        std::cerr << "Socket Creation failed on client side \n";
        return;
    }

    // Initialize client address

    _clientSockAddr.sin_family = AF_INET;
    _clientSockAddr.sin_port = htons(_port);
    memset(&_clientSockAddr.sin_zero, 0, sizeof(_clientSockAddr.sin_zero));

    if (inet_pton(AF_INET, "127.0.0.1", &_clientSockAddr.sin_addr) <= 0)
    {
        std::perror("inet_pton");
        ::close(_clientSockFd);
        _clientSockFd = -1;
        return;
    }

    // Connect client with server

    if (::connect(_clientSockFd,
                  reinterpret_cast<sockaddr *>(&_clientSockAddr),
                  sizeof(_clientSockAddr)) < 0)
    {
        perror("connect");
        return;
    }
    
    setNonBlockingSocket((int &)_clientSockFd);

    _sockBook.emplace_back(pollfd{STDIN_FILENO, POLLIN, 0});
    _sockBook.emplace_back(pollfd{_clientSockFd, POLLIN | POLLOUT, 0});
    _maxFd = 2;
}

bool Client::Tcp::sendMessage(const std::string &msg)
{
    uint32_t netLen = htonl(static_cast<uint32_t>(msg.size()));
    ssize_t n1 = ::send(_clientSockFd, &netLen, sizeof(netLen), MSG_NOSIGNAL);
    if ((n1 != sizeof(netLen)) || (n1<0))
        return false;

    ssize_t n2 = ::send(_clientSockFd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
    return (n2 == static_cast<ssize_t>(msg.size()) && (n2>0));
}

bool Client::Tcp::receiveMessage(std::string &outMsg)
{
    uint32_t netLen = 0;
    if (recv_all(_clientSockFd, &netLen, sizeof(netLen)) <= 0)
        return false;
    uint32_t len = ntohl(netLen);

    const uint32_t kMaxMsg = 8u * 1024u * 1024u; // cap at 8MB
    if (len == 0 || len > kMaxMsg)
    {
        std::cerr << "Invalid message length: " << len << "\n";
        return false;
    }

    outMsg.resize(len);
    if (recv_all(_clientSockFd, &outMsg[0], len) <= 0)
        return false;
    return true;
}

void Client::Tcp::closeConnection()
{
    if (_clientSockFd >= 0)
    {
        ::shutdown(_clientSockFd, SHUT_RDWR);
        ::close(_clientSockFd);
        _clientSockFd = -1;
    }
}

ssize_t Client::Tcp::recv_all(int fd, void *buf, size_t len)
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

void Client::Tcp::chatLoop()
{
    while (true)
    {

        int activity = ::poll(_sockBook.data(), _maxFd, -1);
        if (activity < 0)
            break;

        // Handle user input
        if (_sockBook[0].revents & POLLIN)
        {
            std::string input;
            if (!std::getline(std::cin, input))
                break;
            if (input == "quit")
                break;

            sendMessage(input);
        }
        // Handle server messages
        if (_sockBook[1].revents & POLLIN)
        {

            receiveMessage(_msg);
            std::cout<<"Ack from Server "<<_msg<<std::endl;

        }
    }
    close(_clientSockFd);
}

int main()
{
    Client::Tcp client;
    client.prepareSocketAndConnectToServer();
    client.chatLoop();
}