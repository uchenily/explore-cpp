#include "debug.hpp"

auto main() -> int {
    SET_LOG_LEVEL(print_hpp::log::LogLevel::TRACE);
    LOG_TRACE("hello world");
    LOG_DEBUG("hello world");
    LOG_INFO("hello world");
    LOG_WARN("hello world");
    LOG_ERROR("hello world");
    LOG_FATAL("hello world");

    SET_LOG_STYLE(print_hpp::log::LogStyle::BG);
    LOG_TRACE("hello world");
    LOG_DEBUG("hello world");
    LOG_INFO("hello world");
    LOG_WARN("hello world");
    LOG_ERROR("hello world");
    LOG_FATAL("hello world");

    ASSERT(1 == 1);
    ASSERT_MSG(1 == 1, "1 equals 1");
}
