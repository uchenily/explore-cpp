#include "asio.hpp"
#include "debug.hpp"

using namespace std::literals;
using namespace asio::error;

void print(const asio::error_code &ec, asio::steady_timer *t, int *count) {
    LOG_INFO("callback called, ec={}", ec.message());
    if (*count < 5) {
        LOG_INFO("count={}", *count);
        (*count)++;

        t->expires_at(t->expiry() + 1s);
        t->async_wait([t, count](const asio::error_code &ec) {
            print(ec, t, count);
        });
    }
}

auto main() -> int {
    asio::io_context   io;
    int                count = 0;
    asio::steady_timer t{io, 1s};

    t.async_wait([t = &t, count = &count](const asio::error_code &ec) {
        print(ec, t, count);
    });

    io.run();
    LOG_INFO("Final count is {}", count);
}
