#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include <iostream>

auto main() -> int {
    auto vec = std::vector<int>{1, 2, 3};

    auto result = stdexec::sync_wait(
        stdexec::just(std::move(vec))
        | stdexec::let_value([=](std::vector<int> &totals) {
              totals[0] = 100;
              return stdexec::just(totals);
          }));

    if (result) {
        auto [value] = result.value();
        std::cout << value[0] << '\n';
        std::cout << value[1] << '\n';
        std::cout << value[2] << '\n';
    }
    return 0;
}
