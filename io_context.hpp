#pragma once

#include <cstdint>

class Socket;

class IoContext {
public:
    IoContext();

    void run();

private:
    static constexpr std::size_t max_events = 10;
    const int fd_;
    friend Socket;
    void Attach(Socket* socket);
    void Detach(Socket* socket);
    void WatchRead(Socket* socket);
    void UnwatchRead(Socket* socket);
    void WatchWrite(Socket* socket);
    void UnwatchWrite(Socket* socket);
    void UpdateState(Socket* socket, uint32_t new_state);
};