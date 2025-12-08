#include "Server.hpp"

Server::Tcp::Tcp() : _sockFd(0), _clientSock(-1), _msgLen(0) , _port(32999)
{
    memset(&_sockAddr, 0, sizeof(_sockAddr));
    memset(&_clientSockAddr, 0, sizeof(_clientSockAddr));
    _clientSockSize = sizeof(_clientSockAddr);
}; // defaut port

Server::Tcp::Tcp(uint16_t p_port) : _sockFd(0), _clientSock(-1),  _msgLen(0) , _port(p_port)
{
    memset(&_sockAddr, 0, sizeof(_sockAddr));
    memset(&_clientSockAddr, 0, sizeof(_clientSockAddr));
    _clientSockSize = sizeof(_clientSockAddr);
}; // defined port

void Server::Tcp::prepareSocketAndWaitForClient()
{
    // Create a socket with it's address family , type of socket and with protocol

    _sockFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (_sockFd < 0)
    {
        std::cerr << "Socket creation failed\n";
        return;
    }

    // Initialize address , IP , PORT details
    _sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    _sockAddr.sin_family = AF_INET;
    _sockAddr.sin_port = htons(_port);
    memset(&_sockAddr.sin_zero, 0, sizeof(_sockAddr.sin_zero));

    int yes = 1;
    setsockopt(_sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // Bind the Sockt address with socket

    if (bind(_sockFd, (sockaddr *)&_sockAddr, sizeof(_sockAddr)) < 0)
    {
        std::cerr << "Binding socket address with socket failed\n";
        return;
    }

    // Listening to the client to get connected , means client will try to handshake here

    if (listen(_sockFd, 10) < 0)
    {
        std::cerr << "Listening to the socket failed \n";
        return;
    }
}

void Server::Tcp::acceptClient()
{

    _clientSockSize = sizeof(_clientSockAddr);

    // After handshake accept new incoming clients
    _clientSock = accept(_sockFd, (sockaddr *)&_clientSockAddr, &_clientSockSize);

    if (_clientSock < 0)
    {
        std::cerr << "Client connection failed\n";
        return;
    }

    std::cout << "Client Connected Successully!\n";
}

int Server::Tcp::recvMsgLength()
{
    recv(_clientSock, &_msgLen, sizeof(_msgLen), 0);

    std::cout << "Expecting message of length: " << ntohl(_msgLen) << std::endl;

    return _msgLen;
}

void Server::Tcp::recvMsg()
{

    if(recvMsgLength() < 0)
    {
        std::cerr<<"Invalid message length \n";
        return;
    }
    char *userTempBuffer = new char[_msgLen + 1];
 
    recv(_clientSock, userTempBuffer, _msgLen, 0);

    std::cout<< userTempBuffer << std::endl;
    delete[] userTempBuffer;
}

void Server::Tcp::sendAckToClient()
{
    std::string clientMsg{"Ack to Client from server"};
    uint32_t ackMsgLen = htonl(clientMsg.size());

    send(_clientSock, &ackMsgLen, sizeof(ackMsgLen), 0);
    send(_clientSock, clientMsg.c_str(), clientMsg.size(), 0);
}

void Server::Tcp::closeConnection()
{
    shutdown(_clientSock, SHUT_RDWR);
    close(_clientSock);
}

int main()
{
    Server::Tcp tcpServerInst1;

    tcpServerInst1.prepareSocketAndWaitForClient();
    tcpServerInst1.acceptClient();
    tcpServerInst1.recvMsg();
    tcpServerInst1.sendAckToClient();
    tcpServerInst1.closeConnection();

    return 0;
}