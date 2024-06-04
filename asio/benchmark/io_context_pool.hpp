#pragma once
#include "asio.hpp"
#include <latch>

class IOContextPool final {
    using io_context_ptr = std::unique_ptr<asio::io_context>;
    using work_guard_ptr
        = asio::executor_work_guard<asio::io_context::executor_type>;

public:
    IOContextPool(std::size_t pool_size = std::thread::hardware_concurrency()) {
        for (auto i = 0u; i < pool_size; i++) {
            auto &io_context = io_contexts_.emplace_back(
                // https://think-async.com/Asio/asio-1.30.2/doc/asio/overview/core/concurrency_hint.html
                // FIXME: how to set concurrent hint? it seems that setting it
                // to 1 will compile faster
                std::make_unique<asio::io_context>(1));
            work_guards_.emplace_back(io_context->get_executor());
        }
    }

    ~IOContextPool() {
        stop();
    }

    IOContextPool(const IOContextPool &) = delete;
    auto operator=(const IOContextPool &) = delete;

public:
    auto get_io_context() -> asio::io_context & {
        auto &io_context = *io_contexts_[next_io_context_++];
        if (next_io_context_ == io_contexts_.size()) {
            next_io_context_ = 0;
        }
        return io_context;
    }

    void run() {
        std::vector<std::thread> threads;
        threads.reserve(io_contexts_.size());
        std::latch sync{static_cast<ptrdiff_t>(io_contexts_.size() + 1)};
        for (const auto &context : io_contexts_) {
            threads.emplace_back(
                [&](asio::io_context *ctx) {
                    sync.count_down();
                    ctx->run();
                },
                context.get());
        }
        sync.arrive_and_wait();

        for (auto &thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    void stop() {
        work_guards_.clear();

        for (auto &io_context : io_contexts_) {
            io_context->stop();
        }
    }

private:
    std::size_t                 next_io_context_{};
    std::vector<io_context_ptr> io_contexts_;
    std::vector<work_guard_ptr> work_guards_;
};
