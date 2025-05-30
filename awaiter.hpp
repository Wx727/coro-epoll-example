#pragma once

#include <sys/socket.h>
#include <cerrno>
#include "socket.hpp"

class Accept {
public:
    Accept(Socket* socket) : socket_(socket) {}

    bool await_ready() noexcept {
        return false;
    }

    bool await_suspend(std::coroutine_handle<> h) {
        struct sockaddr_storage addr;
        socklen_t addr_size = sizeof(addr);
        value_ = ::accept(socket_->fd_, (struct sockaddr*)&addr, &addr_size);
        suspend_ = value_ == -1 && (errno == EAGAIN || errno == EWOULDBLOCK);
        // 如果accept失败，返回true让协程挂起，为了后续可以恢复协程，需要保存句柄
        if (suspend_) {
            socket_->coro_ = h;
        }
        return suspend_;
    }

    int await_resume() {
        if (suspend_) {
            struct sockaddr_storage addr;
            socklen_t addr_size = sizeof(addr);
            value_ = ::accept(socket_->fd_, (struct sockaddr*)&addr, &addr_size);
        }
        return value_;
    }

private:
    bool suspend_;
    Socket* socket_;
    int value_;
};

class Recv {
public:
    Recv(Socket* socket, void* buffer, std::size_t len) : socket_(socket),
        buffer_(buffer), len_(len) {
        socket_->io_context_.WatchRead(socket_);
        std::cout << "socket recv operation\n";
    }

    ~Recv() {
        socket_->io_context_.UnwatchRead(socket_);
        std::cout << "~socket recv operation\n";
    }

     bool await_ready() noexcept {
        return false;
    }

    bool await_suspend(std::coroutine_handle<> h) {
        std::cout << "recv fd = " << socket_->fd_ << "\n";
        // 尝试recv，第四个参数是行为控制标志位flags，0表示默认
        value_ = ::recv(socket_->fd_, buffer_, len_, 0);
        suspend_ = value_ == -1 && (errno == EAGAIN || errno == EWOULDBLOCK);
        if (suspend_) {
            socket_->coro_ = h;
        }
        return suspend_;
    }

    ssize_t await_resume() {
        if (suspend_) {
            value_ = ::recv(socket_->fd_, buffer_, len_, 0);
        }
        if (value_ == 0) {
            std::cout << "client closed connection\n";
        } else if (value_ < 0) {
            std::perror("recv");
        }
        return value_;
    }

private:
    bool suspend_;
    Socket* socket_;
    ssize_t value_; // await_resume 的返回值
    void* buffer_;
    std::size_t len_;
};

class Send {
public:
    Send(Socket* socket, void* buffer, std::size_t len) : socket_(socket),
        buffer_(buffer), len_(len) {
        socket_->io_context_.WatchWrite(socket_);
        std::cout << "socket send operation\n";
    }
    
    ~Send() {
        // 必须从 epoll 中移除 fd，否则即使协程已结束（co_return），
        // epoll 仍可能因 fd 可写而触发 resume，导致非法访问已结束协程
        socket_->io_context_.UnwatchWrite(socket_);
        std::cout << "~socket send operation\n";
    }

     bool await_ready() noexcept {
        return false;
    }

    bool await_suspend(std::coroutine_handle<> h) {
        std::cout << "recv fd = " << socket_->fd_ << "\n";
        // 尝试send，第四个参数是行为控制标志位flags，0表示默认
        value_ = ::send(socket_->fd_, buffer_, len_, 0);
        suspend_ = value_ == -1 && (errno == EAGAIN || errno == EWOULDBLOCK);
        if (suspend_) {
            socket_->coro_ = h;
        }
        return suspend_;
    }

    ssize_t await_resume() {
        if (suspend_) {
            value_ = ::send(socket_->fd_, buffer_, len_, 0);
        }
        return value_;
    }

private:
    bool suspend_;
    Socket* socket_;
    ssize_t value_; // await_resume 的返回值
    void* buffer_;
    std::size_t len_;
};