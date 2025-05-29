#include <iostream>
#include "task.hpp"

task<void> foo() {
    std::cout << "foo start\n";
    co_return;
}

task<void> bar() {
    std::cout << "bar start\n";
    co_await foo(); // 嵌套 co_await 测试
    std::cout << "bar end\n";
    co_return;
}

int main() {
    auto t = bar(); // task<void>
    t.resume();     // 手动启动协程
}
