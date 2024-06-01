#if !defined(NDEBUG)
#define NDEBUG
#endif

#include "asio.hpp"
#include "debug.hpp"

using namespace asio::ip;

constexpr std::string_view response = R"(HTTP/1.1 200 OK
Content-Type: text/html; charset=utf-8
Content-Length: 14

Hello, World!
)";

auto process(tcp::socket socket) -> asio::awaitable<void> {
    std::array<char, 128> data;
    while (true) {
        auto [ec, nread] = co_await socket.async_read_some(
            asio::buffer(data),
            asio::as_tuple(asio::use_awaitable));
        if (ec) {
            LOG_ERROR("{}", ec.message());
            co_return;
        }

        LOG_INFO("read {} bytes", nread);
        co_await async_write(socket,
                             asio::buffer(response),
                             asio::use_awaitable);
    }
}

class tcp_server {
public:
    tcp_server(asio::io_context &io_context, uint16_t port)
        : io_context_{io_context}
        , endpoint_{tcp::v4(), port}
        , acceptor_{io_context, endpoint_} {
        LOG_INFO("Listening on {}:{} ...",
                 endpoint_.address().to_string(),
                 port);
        asio::co_spawn(io_context, listen(), asio::detached);
    }

    auto listen() -> asio::awaitable<void> {
        while (true) {
            auto socket = co_await acceptor_.async_accept(asio::use_awaitable);
            [[maybe_unused]] auto remote = socket.remote_endpoint();
            LOG_INFO("new connection: {}:{}",
                     remote.address().to_string(),
                     remote.port());
            asio::co_spawn(io_context_,
                           process(std::move(socket)),
                           asio::detached);
        }
    }

private:
    asio::io_context &io_context_;
    tcp::endpoint     endpoint_;
    tcp::acceptor     acceptor_;
};

auto main() -> int {
    asio::io_context io_context;

    tcp_server server{io_context, 8000};
    io_context.run();
}
