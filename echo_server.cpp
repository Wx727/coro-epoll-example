#include "io_context.hpp"
#include "socket.hpp"

task<> accept(Socket& listen) {
    while(1) {
        std::cout << "accept begin\n";
        auto socket = co_await listen.accept();
        std::cout << "client = " << socket->getfd() << "\n";
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