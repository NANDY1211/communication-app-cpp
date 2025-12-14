#include "Client.hpp"

Client::Tcp::Tcp(uint16_t p_port = 32999) : _clientSockFd(0), _port(p_port), _msgLen(0)
{
    memset(&_clientSockAddr, 0, sizeof(_clientSockAddr));
};

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
    _clientSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    _clientSockAddr.sin_port = htons(_port);
    memset(&_clientSockAddr.sin_zero, 0, sizeof(_clientSockAddr.sin_zero));

    // Connect client with server

    if (::connect(_clientSockFd,
                  reinterpret_cast<sockaddr *>(&_clientSockAddr),
                  sizeof(_clientSockAddr)) < 0)
    {
        perror("connect");
        return;
    }
}

bool Client::Tcp::sendMessage(const std::string &msg)
{
    uint32_t netLen = htonl(static_cast<uint32_t>(msg.size()));
    ssize_t n1 = ::send(_clientSockFd, &netLen, sizeof(netLen), MSG_NOSIGNAL);
    if (n1 != sizeof(netLen))
        return false;

    ssize_t n2 = ::send(_clientSockFd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
    return n2 == static_cast<ssize_t>(msg.size());
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

void Client::Tcp::chatLoop() {
    fd_set readfds;
    while (true) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);      // keyboard input
        FD_SET(_clientSockFd, &readfds);     // server messages
        int maxfd = std::max<int>(STDIN_FILENO, _clientSockFd);

        int sel = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (sel < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        // User typed something
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            std::string msg;
            if (!std::getline(std::cin, msg)) break; // EOF
            if (msg == "quit") break;

            uint32_t len = htonl(msg.size());
            send(_clientSockFd, &len, sizeof(len), 0);
            send(_clientSockFd, msg.c_str(), msg.size(), 0);
        }

        // Server sent something
        if (FD_ISSET(_clientSockFd, &readfds)) {
            uint32_t netLen;
            if (recv_all(_clientSockFd, &netLen, sizeof(netLen)) <= 0) {
                std::cerr << "Server disconnected\n";
                break;
            }
            uint32_t hostLen = ntohl(netLen);
            std::string payload(hostLen, '\0');
            if (recv_all(_clientSockFd, &payload[0], hostLen) <= 0) {
                std::cerr << "Server disconnected\n";
                break;
            }
            std::cout << ">> " << payload << std::endl;
        }
    }
}



int main()
{
    Client::Tcp client;
    client.prepareSocketAndConnectToServer();
    client.chatLoop();
}