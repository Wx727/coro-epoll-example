#pragma once

#include <cstdint>
#include <map>
#include <coroutine>

class Socket;
class Recv;
class Send;

class IoContext {
public:
    friend Socket;
    friend Recv;
    friend Send;

    IoContext();

    void run();

    void Attach(Socket* socket);
    void Detach(Socket* socket);

    // 用于注册等待 I/O 事件的协程
    void WatchRead(Socket* socket, std::coroutine_handle<> h);
    void WatchWrite(Socket* socket, std::coroutine_handle<> h);

    void UnwatchRead(Socket* socket);
    void UnwatchWrite(Socket* socket);

private:
    static constexpr std::size_t max_events = 10;
    const int fd_; // epoll 实例的文件描述符

    // 用于管理等待读事件的协程句柄
    std::map<int, std::coroutine_handle<>> read_waiters_;
    // 用于管理等待写事件的协程句柄
    std::map<int, std::coroutine_handle<>> write_waiters_;

    // 用于跟踪当前的 epoll 事件标志
    std::map<int, uint32_t> current_epoll_flags_;

    void UpdateEpollState(Socket* socket, uint32_t add_event_flag, uint32_t remove_event_flag);
};