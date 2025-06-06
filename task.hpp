#pragma once

#include <coroutine>
#include <cstdlib>

using std::coroutine_handle;
using std::suspend_always;
using std::suspend_never;

template<typename T> struct task;

namespace detail {
template <typename T>
struct promise_type_base {
    coroutine_handle<> continuation_ = std::noop_coroutine();

    task<T> get_return_object();
    
    suspend_always initial_suspend() { return {}; }

    struct final_awaiter {
        bool await_ready() noexcept { return false; }

        void await_resume() noexcept {}

        template<typename promise_type>
        coroutine_handle<> await_suspend(coroutine_handle<promise_type> coro) noexcept {
            return coro.promise().continuation_;
        }
    };

    // 协程在执行完 co_return 后，会隐式地 co_await 其 promise_type::final_suspend() 返回的 final_awaiter
    auto final_suspend() noexcept {
        return final_awaiter{};
    }

    void unhandled_exception() {
        std::exit(-1);
    }
}; // struct promise_type_base

template<typename T>
struct promise_type final : promise_type_base<T> {
    T result;
    // 当协程执行 co_return expression; 并且 expression 不是 void 类型时，return_value() 会被调用
    void return_value(T value) { result = value; }
    task<T> get_return_object();
};

template<>
struct promise_type<void> final : promise_type_base<void> {
    void return_void() {}
    task<void> get_return_object();
};
} // namespace detail

template<typename T = void>
struct task {
    using promise_type = detail::promise_type<T>;
    coroutine_handle<promise_type> handle_;
    
    task(coroutine_handle<promise_type> handle = nullptr) : handle_(handle) {}
    
    bool await_ready() noexcept { return false; }

    coroutine_handle<> await_suspend(coroutine_handle<> waiter) {
        handle_.promise().continuation_ = waiter;
        return handle_;
    }

    T await_resume() { 
        if constexpr (std::is_void_v<T>) {
            return;
        } else {
            return handle_.promise().result;
        }
    }

    void resume() {
        if (handle_)
            handle_.resume();
    }
};

namespace detail {
template<typename T>
inline task<T> promise_type<T>::get_return_object() {
    return task<T>{ coroutine_handle<promise_type<T>>::from_promise(*this) };
}

inline task<void> promise_type<void>::get_return_object() {
    return task<void>{ coroutine_handle<promise_type<void>>::from_promise(*this) };
}
} // namespace detail