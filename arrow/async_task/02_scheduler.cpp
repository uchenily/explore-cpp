#include <arrow/status.h>
#include <arrow/util/async_util.h>
#include <iostream>

using arrow::Future;
using arrow::Status;
using arrow::util::AsyncTaskScheduler;

auto RunMain() -> Status {
    // 初始任务
    auto initial_task = [](AsyncTaskScheduler *scheduler) -> Status {
        std::cout << "Initial task started" << '\n';
        scheduler->AddSimpleTask(
            []() -> Future<> {
                std::cout << "Subtask 1 executed" << '\n';
                return Future<>::MakeFinished();
            },
            std::string{"task1"});
        scheduler->AddSimpleTask(
            []() -> Future<> {
                std::cout << "Subtask 2 executed" << '\n';
                return Future<>::MakeFinished();
            },
            std::string{"task2"});
        return Status::OK();
    };

    // 失败回调
    auto abort_callback = [](const Status &st) {
        std::cerr << "Task failed: " << st.ToString() << '\n';
    };

    // 创建调度器
    Future<> finished = AsyncTaskScheduler::Make(initial_task, abort_callback);

    // 等待调度器完成
    finished.Wait();
    if (finished.status().ok()) {
        std::cout << "Scheduler completed successfully" << '\n';
    } else {
        std::cerr << "Scheduler failed: " << finished.status().ToString()
                  << '\n';
    }

    return Status::OK();
}

auto main() -> int {
    auto status = RunMain();
    if (!status.ok()) {
        std::cerr << "Error occurred: " << status.message() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
