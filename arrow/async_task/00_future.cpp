#include <arrow/api.h>
#include <arrow/util/future.h>
#include <iostream>

using arrow::Future;
using arrow::Status;

auto main() -> int {
    // 创建一个立即完成的 Future，值为 10
    Future<int> future = Future<int>::MakeFinished(10);

    // 链式操作：在 Future 完成后执行回调
    auto final = future
                     .Then([](int value) {
                         std::cout << "First callback: " << value << '\n';
                         return value * 2; // 返回一个新的值
                     })
                     .Then([](int value) {
                         std::cout << "Second callback: " << value << '\n';
                         return Status::OK(); // 返回一个 Status
                     });

    // 等待 Future 链完成
    final.Wait();
    return 0;
}
