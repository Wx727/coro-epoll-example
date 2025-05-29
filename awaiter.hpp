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