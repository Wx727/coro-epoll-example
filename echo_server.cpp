#include "io_context.hpp"
#include "awaiter.hpp"

task<> echo_socket(std::shared_ptr<Socket> socket) {
    while(1) {
        char buffer[1024] = {0};
        ssize_t recv_len = co_await socket->recv(buffer, sizeof(buffer));
        std::cout << "recv buffer = " << buffer << "\n";
        
        ssize_t send_len = 0;
        while(send_len < recv_len) {
            ssize_t ret = co_await socket->send(buffer + send_len, recv_len - send_len);
            if (ret <= 0)
                co_return;
            send_len += ret;
        }
        std::cout << "send done, buffer = " << buffer << "\n";

        // 说明客户端关闭连接（::recv会返回0，而不是-1）
        if (recv_len <= 0) {
            co_return;
        }
    }
}

task<> accept(Socket& listen) {
    while(1) {
        std::cout << "accept begin\n";
        
        // 最后返回值是 task<std::shared_ptr<Socket>>::T await_resume() 来的
        auto socket = co_await listen.accept();
        std::cout << "Socket use_count: " << socket.use_count() << "\n";
        std::cout << "client = " << socket->getfd() << "\n";
        
        auto t = echo_socket(socket);
        std::cout << "Socket use_count: " << socket.use_count() << "\n";
        t.resume();
    }
}

int main() {
    IoContext io_context;
    Socket listen{"8080", io_context};

    std::cout << "listen = " << listen.getfd() << "\n";
    
    auto t = accept(listen);
    t.resume();

    io_context.run();
}