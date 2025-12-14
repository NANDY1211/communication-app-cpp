#include "Client.hpp"



Client::Tcp::Tcp(uint16_t p_port = 32999) : _port(p_port),_clientSockFd(0),_msgLen(2000)
{
    memset(&_clientSockAddr,'\0',sizeof(_clientSockAddr));
}


void Client::Tcp::prepareAndConnectToServer()
{
    //Create a socket

    _clientSockFd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    if(_clientSockFd < 0)
    {
        std::cerr<<"Socket creation failed \n";
        return;
    }

    // Initiaize socket address 

    _clientSockAddr.sin_family = AF_INET;
    _clientSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    _clientSockAddr.sin_port = htons(_port);
    memset(&_clientSockAddr.sin_zero,0,sizeof(_clientSockAddr.sin_zero));

    int yes = 1;
    setsockopt(_clientSockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if(connect(_clientSockFd,(struct sockaddr*)&_clientSockAddr,sizeof(_clientSockAddr)) < 0 )
    {
        std::cerr<<"Failed to connect Server \n";
        return;
    }

}


void Client::Tcp::sendMsg()
{
    std::string frameMsg{"Hi this is client!!"};

    send(_clientSockFd,frameMsg.c_str(),frameMsg.size(),0);
}

void Client::Tcp::recvMsg()
{
    char* tempBuff = new char[2000];

    recv(_clientSockFd,tempBuff,2000,0);

    std::cout<<tempBuff<<std::endl;
}


int main()
{
    Client::Tcp clientInst1;
    clientInst1.prepareAndConnectToServer();
    clientInst1.sendMsg();
    clientInst1.recvMsg();

    return 0;
}