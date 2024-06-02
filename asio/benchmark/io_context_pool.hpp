#include "asio.hpp"

class IOContextPool final {
    using io_context_ptr = std::shared_ptr<asio::io_context>;
    using worker_ptr = std::shared_ptr<asio::io_context::work>;

public:
    IOContextPool(std::size_t pool_size = std::thread::hardware_concurrency()) {
        for (auto i = 0u; i < pool_size; i++) {
            auto &io_context = io_contexts_.emplace_back(
                std::make_shared<asio::io_context>());
            workers_.emplace_back(
                std::make_shared<asio::io_context::work>(*io_context));
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
        for (auto &io_context : io_contexts_) {
            threads_.emplace_back(std::make_shared<std::thread>(
                [](const io_context_ptr &ctx) {
                    ctx->run();
                },
                io_context));
        }
    }

    void wait() {
        for (auto &thread : threads_) {
            if (thread->joinable()) {
                thread->join();
            }
        }
    }

    void stop() {
        workers_.clear();

        for (auto &io_context : io_contexts_) {
            io_context->stop();
        }
    }

private:
    std::size_t                               next_io_context_;
    std::vector<io_context_ptr>               io_contexts_;
    std::vector<worker_ptr>                   workers_;
    std::vector<std::shared_ptr<std::thread>> threads_;
};
