#if !defined(NDEBUG)
#define NDEBUG
#endif

#include "asio.hpp"
#include "debug.hpp"
#include "io_context_pool.hpp"
#include "resource_limit.hpp"
#include "runtime.hpp"

using namespace asio::ip;
using namespace print_hpp::log;

template <typename T>
    requires(!std::is_reference_v<T>)
struct AsioCallbackAwaiter {
public:
    using CallbackFunction
        = std::function<void(std::coroutine_handle<>, std::function<void(T)>)>;

    AsioCallbackAwaiter(CallbackFunction callback_function)
        : callback_function_(std::move(callback_function)) {}

    auto await_ready() noexcept -> bool {
        return false;
    }

    void await_suspend(std::coroutine_handle<> handle) {
        callback_function_(handle, [this](T t) {
            result_ = std::move(t);
        });
    }

    auto await_resume() noexcept -> T {
        return std::move(result_);
    }

private:
    CallbackFunction callback_function_;
    T                result_;
};

inline auto async_accept(tcp::acceptor &acceptor, tcp::socket &socket) noexcept
    -> Task<std::error_code> {
    co_return co_await AsioCallbackAwaiter<std::error_code>{
        [&](std::coroutine_handle<> handle, auto set_resume_value) {
            acceptor.async_accept(
                socket,
                [handle, set_resume_value = std::move(set_resume_value)](
                    auto ec) mutable {
                    set_resume_value(std::move(ec));
                    handle.resume();
                });
        }};
}

template <typename Socket, typename AsioBuffer>
inline auto async_read_some(Socket &socket, AsioBuffer &&buffer) noexcept
    -> Task<std::pair<std::error_code, size_t>> {
    co_return co_await AsioCallbackAwaiter<std::pair<std::error_code, size_t>>{
        [&](std::coroutine_handle<> handle, auto set_resume_value) mutable {
            socket.async_read_some(
                std::forward<AsioBuffer>(buffer),
                [handle,
                 set_resume_value
                 = std::move(set_resume_value)](auto ec, auto size) mutable {
                    set_resume_value(std::make_pair(std::move(ec), size));
                    handle.resume();
                });
        }};
}

template <typename Socket, typename AsioBuffer>
inline auto async_write(Socket &socket, AsioBuffer &&buffer) noexcept
    -> Task<std::pair<std::error_code, size_t>> {
    co_return co_await AsioCallbackAwaiter<std::pair<std::error_code, size_t>>{
        [&](std::coroutine_handle<> handle, auto set_resume_value) mutable {
            asio::async_write(
                socket,
                std::forward<AsioBuffer>(buffer),
                [handle,
                 set_resume_value
                 = std::move(set_resume_value)](auto ec, auto size) mutable {
                    set_resume_value(std::make_pair(std::move(ec), size));
                    handle.resume();
                });
        }};
}

constexpr std::string_view response = R"(HTTP/1.1 200 OK
Content-Type: text/html; charset=utf-8
Content-Length: 14

Hello, World!
)";

auto process(tcp::socket socket) -> Task<void> {
    std::array<char, 128> data;
    while (true) {
        auto [ec, nread] = co_await async_read_some(socket, asio::buffer(data));
        if (ec) {
            LOG_ERROR("{}", ec.message());
            co_return;
        }

        LOG_INFO("read {} bytes", nread);
        co_await async_write(socket, asio::buffer(response));
    }
}

class tcp_server {
public:
    tcp_server(IOContextPool &pool, uint16_t port)
        : pool_{pool}
        , endpoint_{tcp::v4(), port}
        , acceptor_{pool_.get_io_context(), endpoint_} {
        LOG_INFO("Listening on {}:{} ...",
                 endpoint_.address().to_string(),
                 port);
        spawn(listen());
    }

    auto listen() -> Task<void> {
        while (true) {
            auto &ctx = pool_.get_io_context();

            tcp::socket socket(ctx);

            auto ec = co_await async_accept(acceptor_, socket);
            if (ec) [[unlikely]] {
                LOG_ERROR("ec={}", ec.message());
                co_return;
            }

            [[maybe_unused]] auto remote = socket.remote_endpoint();
            LOG_INFO("new connection: {}:{}",
                     remote.address().to_string(),
                     remote.port());
            spawn(process(std::move(socket)));
        }
    }

private:
    IOContextPool &pool_;
    tcp::endpoint  endpoint_;
    tcp::acceptor  acceptor_;
};

auto main() -> int {
    raise_resource_limits();
    SET_LOG_LEVEL(LogLevel::WARN);
    IOContextPool pool{10};
    pool.run();

    tcp_server server{pool, 8000};
    pool.wait();
    restore_resource_limits();
}
