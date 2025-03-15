#include <arrow/api.h>
#include <arrow/util/task_group.h>
#include <arrow/util/thread_pool.h>
#include <chrono>
#include <iostream>
#include <thread>

void Task(int id) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Task " << id << " completed" << std::endl;
}

int main() {
    // 创建一个线程池
    auto thread_pool = arrow::internal::ThreadPool::Make(10).ValueOrDie();

    // 创建一个 TaskGroup
    auto task_group
        = arrow::internal::TaskGroup::MakeThreaded(thread_pool.get());

    // 提交多个任务到 TaskGroup
    for (int i = 0; i < 10; ++i) {
        task_group->Append([i]() {
            Task(i);
            return arrow::Status::OK();
        });
    }

    // 等待所有任务完成
    auto status = task_group->Finish();

    std::cout << "All tasks completed" << std::endl;

    return 0;
}
