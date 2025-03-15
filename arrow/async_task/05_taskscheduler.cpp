#include <arrow/acero/task_util.h>
#include <arrow/acero/util.h>
#include <arrow/status.h>
#include <arrow/util/logging.h>
#include <arrow/util/thread_pool.h>
#include <iostream>
#include <thread>

using namespace arrow;
using namespace arrow::acero;

// 示例任务实现
Status ExampleTaskImpl(size_t thread_index, int64_t task_id) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 模拟任务耗时
    std::cout << "Task " << task_id << " executed by thread " << thread_index
              << std::endl;
    return Status::OK();
}

// 示例任务组 continuation 实现
Status ExampleContinuationImpl(size_t thread_index) {
    std::cout << "Continuation executed by thread " << thread_index
              << std::endl;
    return Status::OK();
}

int main() {
    // 创建 TaskScheduler
    auto scheduler = TaskScheduler::Make();

    // 注册任务组
    int group_id = scheduler->RegisterTaskGroup(ExampleTaskImpl,
                                                ExampleContinuationImpl);
    scheduler->RegisterEnd();

    // 启动任务组
    const int64_t total_num_tasks = 10; // 总任务数
    std::ignore = scheduler->StartTaskGroup(0, group_id, total_num_tasks);

    // 定义调度回调
    // arrow::internal::ThreadPool *thread_pool
    //     = arrow::internal::GetCpuThreadPool();

    auto thread_pool = arrow::internal::ThreadPool::Make(4).ValueUnsafe();

    arrow::acero::ThreadIndexer thread_indexer;

    auto schedule_impl
        // = [](TaskScheduler::TaskGroupContinuationImpl task) -> Status {
        = [&](std::function<Status(size_t)> task) -> Status {
        // 这里可以控制任务的调度逻辑
        // return cont_impl(0); // 使用线程 0 执行 continuation
        return thread_pool->Spawn([&, task]() {
            auto thread_index = thread_indexer();
            ARROW_DCHECK_OK(task(thread_index));
        });
    };

    // 开始调度任务
    // const int num_concurrent_tasks = 4; // 最大并发任务数
    const int num_concurrent_tasks = thread_pool->GetCapacity();

    std::ignore = scheduler->StartScheduling(0,
                                             schedule_impl,
                                             num_concurrent_tasks,
                                             false);

    // 等待任务完成
    std::this_thread::sleep_for(std::chrono::seconds(2)); // 模拟等待

    // 终止调度（如果需要）
    scheduler->Abort([]() {
        std::cout << "Scheduling aborted!" << std::endl;
    });

    return 0;
}
