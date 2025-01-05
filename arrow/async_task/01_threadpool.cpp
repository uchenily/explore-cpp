#include <arrow/api.h>
#include <arrow/util/future.h>
#include <arrow/util/thread_pool.h>
#include <chrono>
#include <iostream>
#include <thread>

using arrow::Future;
using arrow::Status;
using arrow::internal::ThreadPool;
using namespace std::chrono_literals;

auto RunMain() -> Status {
    // 创建一个线程池
    ARROW_ASSIGN_OR_RAISE(auto pool, ThreadPool::Make(4));

    // 提交一个异步任务到线程池
    ARROW_ASSIGN_OR_RAISE(auto future, pool->Submit([]() -> arrow::Result<int> {
        std::this_thread::sleep_for(1s); // 模拟耗时任务
        return 100;
    }));

    // (主线程)等待 Future 完成
    // future.Wait();
    ARROW_ASSIGN_OR_RAISE(auto res, future.result());
    std::cout << "result: " << res << '\n';
    return Status::OK();
}

auto main() -> int {
    auto status = RunMain();
    if (!status.ok()) {
        std::cerr << status.ToString() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
