#pragma once
#include "task.hpp"

inline void spawn(Task<> &&task) {
    auto handle = task.take();
    handle.resume();
}
