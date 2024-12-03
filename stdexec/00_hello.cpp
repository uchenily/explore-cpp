#include <exec/static_thread_pool.hpp>
#include <iostream>
#include <stdexec/execution.hpp>

using namespace stdexec;
using namespace std::string_literals;

auto main() -> int {
    exec::static_thread_pool pool(3);

    auto sched = pool.get_scheduler(); // 1

    auto begin = schedule(sched);                  // 2
    auto hi = then(begin, [] {                     // 3
        std::cout << "Hello world! Have an int."s; // 3
        return 13;                                 // 3
    });                                            // 3
    auto add_42 = then(hi, [](int arg) {
        return arg + 42;
    }); // 4

    auto result = sync_wait(add_42); // 5
    if (result) {
        auto [i] = result.value();
        std::cout << i << '\n';
    }
}
