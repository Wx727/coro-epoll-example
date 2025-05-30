#pragma once

#include <cstdint>

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

private:
    static constexpr std::size_t max_events = 10;
    const int fd_;

    void Attach(Socket* socket);
    void Detach(Socket* socket);
    void WatchRead(Socket* socket);
    void UnwatchRead(Socket* socket);
    void WatchWrite(Socket* socket);
    void UnwatchWrite(Socket* socket);
    void UpdateState(Socket* socket, uint32_t new_state);
};