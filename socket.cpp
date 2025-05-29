#include <netdb.h>
#include <cstring>
#include <stdexcept>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include "socket.hpp"
#include "io_context.hpp"
#include "awaiter.hpp"

Socket::Socket(std::string_view port, IoContext& io_context) : io_context_(io_context) {
    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(nullptr, port.data(), &hints, &res);
    // 显式使用全局命名空间
    fd_ = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    int opt = 1;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (::bind(fd_, res->ai_addr, res->ai_addrlen) == -1) {
        throw std::runtime_error{"bind error"};
    }
    ::listen(fd_, 8);
    fcntl(fd_, F_SETFL, O_NONBLOCK);
    io_context_.Attach(this);
    io_context_.WatchRead(this);
}

Socket::~Socket() {
    if (fd_ == -1)
        return;
    io_context_.Detach(this);
    std::cout << "close fd = " << fd_ << "\n";
    ::close(fd_);
}

bool Socket::resume() {
    if (!coro_) {
        std::cout << "no handle for resume\n";
        return false;
    }
    coro_.resume();
    return true;
}

// 因为promise<T>会先构造，但是Socket没有默认构造函数（没有io_context无法构造），所以这里要使用智能指针包裹
task<std::shared_ptr<Socket>> Socket::accept() {
    int fd = co_await Accept{this};
    if (fd == -1) {
        throw std::runtime_error{"accept error"};
    }
    co_return std::shared_ptr<Socket>(new Socket{fd, io_context_});
}

Socket::Socket(int fd, IoContext& io_context) : fd_(fd), io_context_(io_context) {
    fcntl(fd_, F_SETFL, O_NONBLOCK);
    io_context_.Attach(this);
}