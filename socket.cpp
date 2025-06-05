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
    hints.ai_flags = AI_PASSIVE; // 绑定到所有可用接口

    getaddrinfo(nullptr, port.data(), &hints, &res);
    // 显式使用全局命名空间
    fd_ = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    // 设置 socket 选项，允许端口复用
    int opt = 1;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (::bind(fd_, res->ai_addr, res->ai_addrlen) == -1) {
        throw std::runtime_error{"bind error"};
    }
    freeaddrinfo(res);

    ::listen(fd_, 8);
    // 设置为非阻塞模式
    fcntl(fd_, F_SETFL, O_NONBLOCK);
    io_context_.Attach(this);
}

Socket::Socket(int fd, IoContext& io_context) : fd_(fd), io_context_(io_context) {
    fcntl(fd_, F_SETFL, O_NONBLOCK);
    io_context_.Attach(this);
}

Socket::~Socket() {
    if (fd_ == -1)
        return;
    io_context_.Detach(this);
    std::cout << "close fd = " << fd_ << "\n";
    ::close(fd_);
    fd_ = -1;
}

// 因为promise<T>会先构造，但是Socket没有默认构造函数（没有io_context无法构造），所以这里要使用智能指针包裹，类似于std::optional<T>
task<std::shared_ptr<Socket>> Socket::accept() {
    int client_fd  = co_await Accept{this};
    if (client_fd  == -1) {
        throw std::runtime_error{"accept error"};
    }
    // 这时候调用 promise_type 的 return_value 函数
    // 这个 shared_ptr 会被作为协程的返回值存储在协程的状态中，引用计数+1
    co_return std::shared_ptr<Socket>(new Socket{client_fd, io_context_});
}

Recv Socket::recv(void* buffer, std::size_t len) {
    return Recv{this, buffer, len};
}

Send Socket::send(void* buffer, std::size_t len) {
    return Send{this, buffer, len};
}