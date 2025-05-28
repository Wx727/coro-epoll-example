#include <iostream>
#include "task.h"

task<int> foo() {
    std::cout << "foo start\n";
    co_return 42;
}

task<void> bar() {
    std::cout << "bar start\n";
    int x = co_await foo(); // 嵌套 co_await 测试
    std::cout << "got from foo: " << x << "\n";
    co_return;
}

int main() {
    auto t = bar(); // task<void>
    t.resume();     // 手动启动协程
}
