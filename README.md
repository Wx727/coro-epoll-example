# coro-epoll-example
使用 C++20 协程和 epoll 实现的 Echo Server 示例。

编译与测试：
```
mkdir build && cd build
cmake ..
make
./echo_server &     # 启动服务器

nc 127.0.0.1 8080   # 客户端连接测试
```

学习过程中所写，参考（抄）自：https://github.com/franktea/coro_epoll_kqueue