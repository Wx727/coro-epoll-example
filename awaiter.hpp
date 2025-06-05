#pragma once

#include <sys/socket.h>
#include <cerrno>
#include "socket.hpp"

class Accept {
public:
    Accept(Socket* socket) : socket_(socket) {}

    bool await_ready() noexcept {
        struct sockaddr_storage addr;
        socklen_t addr_size = sizeof(addr);
        value_ = ::accept(socket_->getfd(), (struct sockaddr*)&addr, &addr_size);
        // 如果 accept 成功，或者出现除 EAGAIN/EWOULDBLOCK 之外的错误，则不挂起
        return value_ != -1 || (errno != EAGAIN && errno != EWOULDBLOCK);
    }

    // 如果 await_ready 返回 false，则协程将在此挂起
    bool await_suspend(std::coroutine_handle<> h) {
        // 将当前协程句柄注册到 IoContext，等待读事件（新连接是读事件）
        socket_->io_context_.WatchRead(socket_, h);
        return true; // 返回 true 表示协程需要挂起
    }

    int await_resume() {
        // 如果 await_ready 已经成功获取值，直接返回
        // 否则，在被 epoll 唤醒恢复后，再尝试获取一次（边缘触发特性）
        if (value_ == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            struct sockaddr_storage addr;
            socklen_t addr_size = sizeof(addr);
            value_ = ::accept(socket_->getfd(), (struct sockaddr*)&addr, &addr_size);
        }

        return value_; // 返回接受的客户端 fd
    }

private:
    Socket* socket_;
    int value_;
};

class Recv {
public:
    Recv(Socket* socket, void* buffer, std::size_t len) : socket_(socket),
        buffer_(buffer), len_(len) {
    }

    bool await_ready() noexcept {
        value_ = ::recv(socket_->getfd(), buffer_, len_, 0);
        return value_ != -1 || (errno != EAGAIN && errno != EWOULDBLOCK);
    }

    bool await_suspend(std::coroutine_handle<> h) {
        socket_->io_context_.WatchRead(socket_, h);
        return true;
    }

    ssize_t await_resume() {
        if (value_ == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            value_ = ::recv(socket_->getfd(), buffer_, len_, 0);
        }

        return value_;
    }

private:
    Socket* socket_;
    ssize_t value_; // await_resume 的返回值（接收字节数）
    void* buffer_; // 接收缓冲区
    std::size_t len_; // 接收缓冲区长度
};

class Send {
public:
    Send(Socket* socket, void* buffer, std::size_t len) : socket_(socket),
        buffer_(buffer), len_(len), value_(0) {}
    
    bool await_ready() noexcept {
        value_ = ::send(socket_->getfd(), buffer_, len_, 0);
        return value_ != -1 || (errno != EAGAIN && errno != EWOULDBLOCK);
    }

    bool await_suspend(std::coroutine_handle<> h) {
        socket_->io_context_.WatchWrite(socket_, h);
        return true;
    }

    ssize_t await_resume() {
        if (value_ == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            value_ = ::send(socket_->getfd(), buffer_, len_, 0);
        }

        return value_;
    }

private:
    Socket* socket_;
    ssize_t value_;
    void* buffer_;
    std::size_t len_;
};