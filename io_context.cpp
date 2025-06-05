#include <sys/epoll.h>
#include <stdexcept>
#include "io_context.hpp"
#include "socket.hpp"

IoContext::IoContext() : fd_(epoll_create1(0)) {
    if (fd_ == -1) {
        throw std::runtime_error{"epoll_create1"};
    }
}

void IoContext::run() {
    struct epoll_event events[max_events];
    while (true) {
        int nfds = epoll_wait(fd_, events, max_events, -1);
        if (nfds == -1) {
            throw std::runtime_error{"epoll_wait"};
        }

        for (int i = 0; i < nfds; i++) {
            auto socket = static_cast<Socket*>(events[i].data.ptr);
            int event_fd = socket->getfd();
            // 处理读事件
            if (events[i].events & EPOLLIN) {
                auto it = read_waiters_.find(event_fd);
                if (it != read_waiters_.end()) {
                    std::coroutine_handle<> h = it->second;
                    read_waiters_.erase(it);
                    if (h && !h.done()) {
                        h.resume();
                    }
                }
            }
            // 处理写事件
            if (events[i].events & EPOLLOUT) {
                auto it = write_waiters_.find(event_fd);
                if (it != write_waiters_.end()) {
                    std::coroutine_handle<> h = it->second;
                    write_waiters_.erase(it);
                    if (h && !h.done()) {
                        h.resume();
                    }
                }
            }
        }
    }
}

void IoContext::Attach(Socket* socket) {
    struct epoll_event ev;
    ev.events = EPOLLET;
    ev.data.ptr = socket;
    if (epoll_ctl(fd_, EPOLL_CTL_ADD, socket->getfd(), &ev) == -1) {
        throw std::runtime_error{"epoll_ctl: attach"};
    }
    current_epoll_flags_[socket->getfd()] = 0; // 初始时，fd_flags 没有 IN 或 OUT 标志
}

void IoContext::Detach(Socket* socket) {
    int target_fd = socket->getfd();
    if (epoll_ctl(fd_, EPOLL_CTL_DEL, target_fd, nullptr) == -1) {
        throw std::runtime_error{"epoll_ctl: detach"};
    }
    current_epoll_flags_.erase(target_fd); // 移除状态记录
    read_waiters_.erase(target_fd); // 确保清理任何残留的等待者
    write_waiters_.erase(target_fd);
}

void IoContext::UpdateEpollState(Socket* socket, uint32_t add_event_flag, uint32_t remove_event_flag) {
    int target_fd = socket->getfd();

    // 从内部状态中获取当前 fd 的 epoll 事件标志
    // 如果 fd 不存在（不应该发生，除非 Attach 失败或 Detach 过早），则默认 0
    uint32_t old_flags = current_epoll_flags_.count(target_fd) ? current_epoll_flags_[target_fd] : 0;

    uint32_t new_flags = old_flags;
    if (add_event_flag != 0) {
        new_flags |= add_event_flag;
    }
    if (remove_event_flag != 0) {
        new_flags &= ~remove_event_flag;
    }
    
    if (new_flags == old_flags) {
        return;
    }

    struct epoll_event ev;
    ev.events = new_flags | EPOLLET; // 始终保持边缘触发模式
    ev.data.ptr = socket; // 在 MOD 操作时，data.ptr 必须和 ADD 时一致

    if (epoll_ctl(fd_, EPOLL_CTL_MOD, target_fd, &ev) == -1) {
        throw std::runtime_error{"epoll_ctl: mod"};
    }
    current_epoll_flags_[target_fd] = new_flags; // 更新内部状态记录
}

void IoContext::WatchRead(Socket* socket, std::coroutine_handle<> h) {
    read_waiters_[socket->getfd()] = h; // 存储协程句柄
    UpdateEpollState(socket, EPOLLIN, 0);
}

void IoContext::UnwatchRead(Socket* socket) {
    // 只有当确信不再需要监听读事件时才调用
    // 在协程恢复后，其句柄会自动从 map 中移除，但 epoll 监听状态可能仍然存在
    // 这里用于显式移除 EPOLLIN 监听
    UpdateEpollState(socket, 0, EPOLLIN); // 移除 EPOLLIN 标志
    read_waiters_.erase(socket->getfd()); // 确保从 map 中移除
}

void IoContext::WatchWrite(Socket* socket, std::coroutine_handle<> h) {
    write_waiters_[socket->getfd()] = h;
    UpdateEpollState(socket, EPOLLOUT, 0);
}

void IoContext::UnwatchWrite(Socket* socket) {
    // 同 UnwatchRead
    UpdateEpollState(socket, 0, EPOLLOUT);
    write_waiters_.erase(socket->getfd());
}