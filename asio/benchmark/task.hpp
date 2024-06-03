#pragma once

#include <cassert>
#include <coroutine>
#include <exception>
#include <optional>

template <typename T>
class Task;

namespace detail {
class TaskPromiseBase {
    struct FinalAwaiter {
        auto await_ready() const noexcept -> bool {
            return false;
        }
        template <typename TaskPromise>
        auto
        await_suspend(std::coroutine_handle<TaskPromise> callee) const noexcept
            -> std::coroutine_handle<> {
            if (callee.promise().caller_ != nullptr) {
                return callee.promise().caller_;
            } else {
                callee.destroy();
                return std::noop_coroutine();
            }
        }
        auto await_resume() const noexcept {}
    };

public:
    // auto get_return_object() noexcept -> Task<?>;

    auto initial_suspend() const noexcept -> std::suspend_always {
        return {};
    }

    auto final_suspend() noexcept {
        return FinalAwaiter{};
    }

    auto unhandled_exception() {
        std::terminate();
    }

public:
    std::coroutine_handle<> caller_{nullptr};
};

template <typename T>
class TaskPromise final : public TaskPromiseBase {
public:
    inline auto get_return_object() noexcept -> Task<T>;

    template <typename F>
    auto return_value(F &&value) noexcept {
        value_ = std::forward<F>(value);
    }

    auto result() const noexcept {
        assert(value_.has_value());
        return value_.value();
    }

private:
    std::optional<T> value_{std::nullopt};
};

template <>
class TaskPromise<void> final : public TaskPromiseBase {
public:
    inline auto get_return_object() noexcept -> Task<void>;

    auto return_void() const noexcept {}

    auto result() const noexcept {}
};

} // namespace detail

template <typename T = void>
class Task {
public:
    using promise_type = detail::TaskPromise<T>;

    Task(std::coroutine_handle<promise_type> handle)
        : handle_{handle} {}

    ~Task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    // 为什么不定义移动构造函数就coredump了?
    Task(Task &&other) noexcept
        : handle_{std::move(other.handle_)} {
        other.handle_ = nullptr;
    }
    auto operator=(Task &&other) noexcept -> Task & {
        if (std::addressof(other) != this) {
            handle_ = std::move(other.handle_);
            other.handle_ = nullptr;
        }
        return *this;
    }

    Task(const Task &) = delete;
    auto operator=(const Task &) -> Task & = delete;

public:
    auto operator co_await() const & noexcept {
        struct Awaitable {
            std::coroutine_handle<promise_type> callee_;

            Awaitable(std::coroutine_handle<promise_type> callee) noexcept
                : callee_{callee} {}

            auto await_ready() const noexcept -> bool {
                return !callee_ || callee_.done();
            }

            auto await_suspend(std::coroutine_handle<> caller)
                -> std::coroutine_handle<> {
                callee_.promise().caller_ = caller;
                return callee_;
            }

            auto await_resume() const noexcept {
                assert(callee_ && "no callee");
                return callee_.promise().result();
            }
        };
        return Awaitable{handle_};
    }

    auto take() -> std::coroutine_handle<promise_type> {
        if (handle_ == nullptr) [[unlikely]] {
            std::terminate();
        }
        auto res = std::move(handle_);
        handle_ = nullptr;
        return res;
    }

    auto resume() {
        handle_.resume();
    }

private:
    std::coroutine_handle<promise_type> handle_;
};

template <typename T>
inline auto detail::TaskPromise<T>::get_return_object() noexcept -> Task<T> {
    return Task<T>{std::coroutine_handle<TaskPromise>::from_promise(*this)};
}

inline auto detail::TaskPromise<void>::get_return_object() noexcept
    -> Task<void> {
    return Task<void>{std::coroutine_handle<TaskPromise>::from_promise(*this)};
}
