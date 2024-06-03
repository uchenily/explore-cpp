#pragma once
#include "task.hpp"

inline void spawn(Task<> &&task) {
    auto main_coro = [](Task<> task) -> Task<void> {
        co_await task;
    }(std::move(task));

    auto handle = main_coro.take();
    handle.resume();
}
