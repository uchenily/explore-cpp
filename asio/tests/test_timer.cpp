#include "asio.hpp"
#include "debug.hpp"

using namespace std::literals;

auto main() -> int {
    asio::io_context   io;
    asio::steady_timer t{io, 5s};

    LOG_INFO("before wait()");
    // 阻塞等待
    t.wait();
    LOG_INFO("after wait()");
}
