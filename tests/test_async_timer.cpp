#include "asio.hpp"
#include "debug.hpp"

using namespace std::literals;

auto main() -> int {
    asio::io_context   io;
    asio::steady_timer t{io, 5s};

    LOG_INFO("before async_wait()");
    t.async_wait([](const asio::error_code &ec) {
        LOG_INFO("callback called, ec={}", ec.message());
    });
    LOG_INFO("after async_wait()");

    // 记住在调用 io.run()之前, 给io_context 一些活干, 不然就会立即退出
    // 在这个例子中, 就是给io_context async_wait 这个活, 只有等这个活干完了,
    // 这个程序才会结束
    io.run();
}
