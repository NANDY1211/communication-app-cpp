#include "Client.hpp"

Client::Tcp::Tcp() : _clientSockFd(0), _port(32999), _msgLen(0)
{
    memset(&_clientSockAddr, 0, sizeof(_clientSockAddr));
};

Client::Tcp::Tcp(uint16_t p_port) : _clientSockFd(0), _port(p_port), _msgLen(0)
{
    memset(&_clientSockAddr, 0, sizeof(_clientSockAddr));
};

void Client::Tcp::prepareSocketAndConnectToServer()
{
    // Create a socket with address family , socket type and protocol
    _clientSockFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

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

    if (connect(_clientSockFd, (struct sockaddr *)&_clientSockAddr, sizeof(_clientSockAddr)) < 0)
    {
        std::cerr << "connecting to server failed \n";
        return;
    }
}

void Client::Tcp::sendMsgToServer()
{
    std::string clientMsg{"Hi , this is Client"};
    uint32_t msgLen = htonl(clientMsg.size());

    send(_clientSockFd, &msgLen, sizeof(msgLen), 0);          // sending length first to server
    send(_clientSockFd, clientMsg.c_str(), clientMsg.size(), 0); // sending the actual message
}

int Client::Tcp::recvMsgLength()
{
    recv(_clientSockFd, &_msgLen, sizeof(_msgLen), 0);

    std::cout << "Expecting message of length: " << ntohl(_msgLen) << std::endl;
    return _msgLen;
}

void Client::Tcp::recvMsg()
{
    if (recvMsgLength() < 0)
    {
        std::cerr << "Receive message failed \n";
        return;
    }
    char *userTempBuffer = new char[_msgLen + 1];
    memset(userTempBuffer, '\0', _msgLen + 1);

    recv(_clientSockFd, userTempBuffer, _msgLen, 0);

    std::cout << userTempBuffer << std::endl;

    delete[] userTempBuffer;
}

void Client::Tcp::closeConnection()
{
    shutdown(_clientSockFd, SHUT_RDWR);
    close(_clientSockFd);
}

int main()
{
    Client::Tcp clientIns1;
    clientIns1.prepareSocketAndConnectToServer();
    clientIns1.sendMsgToServer();
    clientIns1.recvMsg();
    clientIns1.closeConnection();
}