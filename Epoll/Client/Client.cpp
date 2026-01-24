#include "Client.hpp"

#include <iostream>
#include <cstring>
#include <cerrno>

using namespace epoll_client;

static constexpr int MAX_EVENTS = 16;

TcpClient::TcpClient(uint16_t port)
    : serverPort_(port),
      sockFd_(-1),
      epollFd_(-1),
      events_(MAX_EVENTS) {}

TcpClient::~TcpClient() {
    if (sockFd_ >= 0) close(sockFd_);
    if (epollFd_ >= 0) close(epollFd_);
}

void TcpClient::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) flags = 0;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void TcpClient::connectToServer() {
    sockFd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockFd_ < 0) {
        perror("socket");
        std::exit(EXIT_FAILURE);
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(serverPort_);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (connect(sockFd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        std::exit(EXIT_FAILURE);
    }

    setNonBlocking(sockFd_);

    int one = 1;
    setsockopt(sockFd_, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    epollFd_ = epoll_create1(0);
    if (epollFd_ < 0) {
        perror("epoll_create1");
        std::exit(EXIT_FAILURE);
    }

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = sockFd_;
    epoll_ctl(epollFd_, EPOLL_CTL_ADD, sockFd_, &ev);

    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    epoll_ctl(epollFd_, EPOLL_CTL_ADD, STDIN_FILENO, &ev);

    std::cout << "Connected to " << serverIp_ << ":" << serverPort_ << std::endl;
}

void TcpClient::sendMessage(const std::string& msg) {
    if (msg.empty()) return;
    send(sockFd_, msg.c_str(), msg.size(), MSG_NOSIGNAL);
}

bool TcpClient::receiveMessage() {
    char buffer[4096];
    ssize_t n = recv(sockFd_, buffer, sizeof(buffer), 0);

    if (n <= 0)
        return false;

    std::cout << "Server: " << std::string(buffer, buffer + n);
    std::cout.flush();
    return true;
}

void TcpClient::run() {
    while (true) {
        int n = epoll_wait(epollFd_, events_.data(), events_.size(), -1);
        if (n < 0) {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; ++i) {
            int fd = events_[i].data.fd;
            uint32_t ev = events_[i].events;

            if (ev & (EPOLLERR | EPOLLHUP)) {
                std::cout << "Disconnected from server\n";
                return;
            }

            if (fd == STDIN_FILENO) {
                std::string line;
                if (!std::getline(std::cin, line))
                    return;
                line.push_back('\n');
                sendMessage(line);
            }
            else if (fd == sockFd_) {
                if (!receiveMessage()) {
                    std::cout << "Server closed connection\n";
                    return;
                }
            }
        }
    }
}


int main(){
    epoll_client::TcpClient client(17888);
    client.connectToServer();
    client.run();
}