#pragma once

#include "log.hpp"
#include "print.hpp"

inline auto &debug_logger = print_hpp::log::console;

#if !defined(NDEBUG)

#define SET_LOG_LEVEL(level) debug_logger.set_level(level)
#define SET_LOG_STYLE(style) debug_logger.set_style(style)
#define LOG_TRACE(...) debug_logger.trace(__VA_ARGS__)
#define LOG_INFO(...) debug_logger.info(__VA_ARGS__)
#define LOG_DEBUG(...) debug_logger.debug(__VA_ARGS__)
#define LOG_WARN(...) debug_logger.warn(__VA_ARGS__)
#define LOG_ERROR(...) debug_logger.error(__VA_ARGS__)
#define LOG_FATAL(...) debug_logger.fatal(__VA_ARGS__)

#define PRINT(...) print_hpp::print(__VA_ARGS__)

#else

#define SET_LOG_LEVEL(level)
#define SET_LOG_STYLE(style)
#define LOG_TRACE(...)
#define LOG_INFO(...)
#define LOG_DEBUG(...)
#define LOG_WARN(...)
#define LOG_ERROR(...)
#define LOG_FATAL(...)

#define PRINT(...)

#endif

#if !defined(NDEBUG)

#include <cassert>
#define ASSERT(x) assert(x)
#define ASSERT_MSG(x, msg) ASSERT((x) && (msg))

#else

#define ASSERT(x)
#define ASSERT_MSG(x, msg)

#endif