#include "asio.hpp"
#include "debug.hpp"

using namespace std::literals;

class printer {
public:
    printer(asio::io_context &io)
        : strand_{asio::make_strand(io)}
        , timer1_{io, 1s}
        , timer2_{io, 1s}
        , count_{0} {
        timer1_.async_wait([this](auto) {
            print1();
        });
        timer2_.async_wait([this](auto) {
            print2();
        });
    }

    ~printer() {
        LOG_INFO("Final count is {}", count_);
    }

    void print1() {
        if (count_ < 10) {
            LOG_INFO("timer1 count={}", count_);
            count_++;

            timer1_.expires_at(timer1_.expiry() + 1s);
            timer1_.async_wait([this](auto) {
                print1();
            });
        }
    }

    void print2() {
        if (count_ < 10) {
            LOG_INFO("timer2 count={}", count_);
            count_++;

            timer2_.expires_at(timer2_.expiry() + 1s);
            timer2_.async_wait([this](auto) {
                print2();
            });
        }
    }

private:
    asio::strand<asio::io_context::executor_type> strand_;
    asio::steady_timer                            timer1_;
    asio::steady_timer                            timer2_;
    int                                           count_;
};

auto main() -> int {
    asio::io_context io;

    printer p{io};
    // 现在有两个线程: 主线程, 和一个额外的线程
    std::thread t{[&io]() {
        io.run();
    }};
    // 在所有异步操作完成之前, 后台线程 t 不会退出
    io.run(); // 现在两个线程中都在(并发地)运行 io.run()
    t.join();
}
