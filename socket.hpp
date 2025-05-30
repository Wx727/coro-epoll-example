#pragma once

#include <cstdint>
#include <string_view>
#include <memory>
#include <iostream>
#include "task.hpp"

class IoContext;
class Accept;
class Recv;
class Send;

class Socket {
public:
    friend IoContext;
    friend Accept;
    friend Recv;
    friend Send;

    Socket(std::string_view port, IoContext& io_context);

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    ~Socket();

    bool resume();

    task<std::shared_ptr<Socket>> accept();

    Recv recv(void* buffer, std::size_t len);

    Send send(void* buffer, std::size_t len);

    int getfd() {
        return fd_;
    }

private:
    explicit Socket(int fd, IoContext& io_context);

private:
    int fd_ = -1;
    IoContext& io_context_;
    uint32_t io_state = 0;
    std::coroutine_handle<> coro_;
};