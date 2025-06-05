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
    friend Accept;
    friend Recv;
    friend Send;

    Socket(std::string_view port, IoContext& io_context);

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    ~Socket();

    // 返回一个 awaitable task，用于接受新的连接
    task<std::shared_ptr<Socket>> accept();

    // 返回一个 awaitable 对象，用于接收数据
    Recv recv(void* buffer, std::size_t len);

    // 返回一个 awaitable 对象，用于发送数据
    Send send(void* buffer, std::size_t len);

    int getfd() const {
        return fd_;
    }

private:
    // 私有构造函数用于创建已接受的客户端 Socket
    explicit Socket(int fd, IoContext& io_context);

private:
    int fd_ = -1;
    IoContext& io_context_;
};