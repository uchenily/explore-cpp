#include <cstdio>
#include <stdexec/execution.hpp>
#include <thread>

using namespace std::literals;

auto main() -> int {
    stdexec::run_loop loop;

    std::jthread worker([&](const std::stop_token &st) {
        std::stop_callback cb{st, [&] {
                                  loop.finish();
                              }};
        loop.run();
    });

    /*stdexec::sender*/ auto hello = stdexec::just("hello world"s);
    /*stdexec::sender*/ auto print
        = std::move(hello) | stdexec::then([](auto msg) {
              std::puts(msg.c_str());
              return 0;
          });

    /*stdexec::scheduler*/ auto io_thread = loop.get_scheduler();

    /*stdexec::sender*/ auto work
        = stdexec::start_on(io_thread, std::move(print));

    auto [result] = stdexec::sync_wait(std::move(work)).value();

    return result;
}
