#pragma once

#include "debug.hpp"
#include <sys/resource.h>

static struct rlimit initial_fd_limit {};

inline void raise_resource_limits() {
    if (getrlimit(RLIMIT_NOFILE, &initial_fd_limit) < 0) {
        LOG_ERROR("Failed to get file descriptor limit");
    }

    struct rlimit new_limit = initial_fd_limit;
    new_limit.rlim_cur = std::max<rlim_t>(new_limit.rlim_cur, 65536);
    if (new_limit.rlim_max != RLIM_INFINITY) {
        new_limit.rlim_cur
            = std::min<rlim_t>(new_limit.rlim_cur, new_limit.rlim_max);
    }
    if (new_limit.rlim_cur != initial_fd_limit.rlim_cur) {
        if (setrlimit(RLIMIT_NOFILE, &new_limit) < 0) {
            LOG_ERROR("Failed to set file descriptor limit");
        }
    }
}

inline void restore_resource_limits() {
    if (initial_fd_limit.rlim_cur == 0 && initial_fd_limit.rlim_max == 0) {
        return;
    }
    if (setrlimit(RLIMIT_NOFILE, &initial_fd_limit) < 0) {
        LOG_ERROR("Failed to set file descriptor limit");
    }
}
