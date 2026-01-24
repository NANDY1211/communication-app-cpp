#include "Server.hpp"

using namespace epoll_server;

static constexpr int MAX_EVENTS = 1024;
static constexpr int LISTEN_BACKLOG = 128;

Tcp::Tcp(uint16_t port)
    : port_(port),
      masterFd_(-1),
      epollFd_(-1),
      clientLen_(sizeof(clientAddr_)),
      events_(MAX_EVENTS) {}

Tcp::~Tcp()
{
    if (masterFd_ >= 0)
        close(masterFd_);
    if (epollFd_ >= 0)
        close(epollFd_);
}

void Tcp::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        flags = 0;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Tcp::closeFd(int fd)
{
    epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
}

void Tcp::initSocketAndListen()
{
    masterFd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (masterFd_ < 0)
    {
        perror("socket");
        std::exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(masterFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    serverAddr_.sin_family = AF_INET;
    serverAddr_.sin_addr.s_addr = INADDR_ANY;
    serverAddr_.sin_port = htons(port_);

    if (bind(masterFd_, (sockaddr *)&serverAddr_, sizeof(serverAddr_)) < 0)
    {
        perror("bind");
        std::exit(EXIT_FAILURE);
    }

    if (listen(masterFd_, LISTEN_BACKLOG) < 0)
    {
        perror("listen");
        std::exit(EXIT_FAILURE);
    }

    setNonBlocking(masterFd_);

    epollFd_ = epoll_create1(0);
    if (epollFd_ < 0)
    {
        perror("epoll_create1");
        std::exit(EXIT_FAILURE);
    }

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = masterFd_;

    epoll_ctl(epollFd_, EPOLL_CTL_ADD, masterFd_, &ev);

    std::cout << "Server listening on port " << port_ << std::endl;
}

void Tcp::acceptConnections()
{
    while (true)
    {
        int clientFd = accept(masterFd_, (sockaddr *)&clientAddr_, &clientLen_);
        if (clientFd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            perror("accept");
            break;
        }

        setNonBlocking(clientFd);

        int one = 1;
        setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = clientFd;

        epoll_ctl(epollFd_, EPOLL_CTL_ADD, clientFd, &ev);

        std::cout << "Client connected: fd=" << clientFd << std::endl;
    }
}

bool Tcp::receiveAndProcess(int fd)
{
    char buffer[4096];
    ssize_t n = recv(fd, buffer, sizeof(buffer), 0);

    if (n <= 0)
    {
        return false;
    }

    std::string msg(buffer, buffer + n);
    std::cout << "Received from fd=" << fd << ": " << msg << std::endl;

    const std::string reply = "Ack from Server\n";
    send(fd, reply.c_str(), reply.size(), MSG_NOSIGNAL);

    return true;
}

void Tcp::run()
{
    while (true)
    {
        int n = epoll_wait(epollFd_, events_.data(), events_.size(), -1);
        if (n < 0)
        {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; ++i)
        {
            int fd = events_[i].data.fd;
            uint32_t ev = events_[i].events;

            if (ev & (EPOLLERR | EPOLLHUP))
            {
                closeFd(fd);
                continue;
            }

            if (fd == masterFd_)
            {
                acceptConnections();
            }
            else if (ev & EPOLLIN)
            {
                if (!receiveAndProcess(fd))
                {
                    closeFd(fd);
                }
            }
        }
    }
}

int main()
{
    epoll_server::Tcp servInst;
    servInst.initSocketAndListen();
    servInst.run();
    return 0;
}
