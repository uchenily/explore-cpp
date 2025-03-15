#include "arrow/compute/exec.h"
#include <arrow/status.h>
#include <arrow/util/async_util.h>
#include <arrow/util/future.h>
#include <iostream>

using namespace arrow;

// 自定义任务类
class MyTask : public util::AsyncTaskScheduler::Task {
public:
    Result<Future<>> operator()() override {
        // 模拟任务提交
        std::cout << "Task submitted: " << name() << std::endl;
        return Future<>::Make();
    }

    std::string_view name() const override {
        return "MyTask";
    }
};

template <typename Callable>
class CallableTask : public util::AsyncTaskScheduler::Task {
public:
    CallableTask(Callable callable)
        : callable_(std::move(callable)) {}
    Result<Future<>> operator()() override {
        return callable_();
    }

    std::string_view name() const override {
        return "CallableTask";
    }

private:
    Callable callable_;
};

int main() {
    auto thread_pool = arrow::internal::ThreadPool::Make(4).ValueOrDie();
    // 创建一个调度器
    auto scheduler_future = util::AsyncTaskScheduler::Make(
        [&](util::AsyncTaskScheduler *scheduler) -> Status {
            // 添加自定义任务
            // scheduler->AddTask(std::make_unique<MyTask>());
            auto func = [&]() {
                return thread_pool
                    ->Submit([]() {
                        std::cout << "Task finished\n";
                    })
                    .ValueOrDie();

                // return Future<>::Make(); // 不会结束
                // return Future<>::
                //     MakeFinished(); // 只有任务返回MakeFinished才会结束
            };
            scheduler->AddTask(
                std::make_unique<CallableTask<decltype(func)>>(func));
            return Status::OK();
        },
        [](const Status &status) {
            // 中止回调
            std::cerr << "Scheduler aborted: " << status.ToString()
                      << std::endl;
        });

    // 等待调度器完成
    scheduler_future.Wait();

    return 0;
}
