#include "asio.hpp"
#include "debug.hpp"

using namespace std::literals;

class printer {
public:
    printer(asio::io_context &io)
        : timer_{io, 1s}
        , count_{0} {
        timer_.async_wait([this](auto) {
            print();
        });
    }

    ~printer() {
        LOG_INFO("Final count is {}", count_);
    }

    void print() {
        if (count_ < 5) {
            LOG_INFO("count={}", count_);
            count_++;

            timer_.expires_at(timer_.expiry() + 1s);
            timer_.async_wait([this](auto) {
                print();
            });
        }
    }

private:
    asio::steady_timer timer_;
    int                count_;
};
auto main() -> int {
    asio::io_context io;

    printer p{io};
    io.run();
}
