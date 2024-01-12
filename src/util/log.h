// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_LOG_H
#define BITCOIN_UTIL_LOG_H

#include <attributes.h>
#include <logging/categories.h> // IWYU pragma: export
#include <tinyformat.h>
#include <util/check.h>

#include <cstdint>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>

/// Like std::source_location, but allowing to override the function name.
class SourceLocation
{
public:
    /// The func argument must be constructed from the C++11 __func__ macro.
    /// Ref: https://en.cppreference.com/w/cpp/language/function.html#func
    /// Non-static string literals are not supported.
    SourceLocation(const char* func,
                   std::source_location loc = std::source_location::current())
        : m_func{func}, m_loc{loc} {}

    std::string_view file_name() const { return m_loc.file_name(); }
    std::uint_least32_t line() const { return m_loc.line(); }
    std::string_view function_name_short() const { return m_func; }

private:
    std::string_view m_func;
    std::source_location m_loc;
};

namespace util::log {
/** Opaque to util::log; interpreted by consumers (e.g., BCLog::LogFlags). */
using Category = uint64_t;

/** Base class inherited by log consumers. Opaque like category, used for basic type-checking. */
class Logger{};

enum class Level {
    Trace = 0, // High-volume or detailed logging for development/debugging
    Debug,     // Reasonably noisy logging, but still usable in production
    Info,      // Default
    Warning,
    Error,
};

struct Entry {
    Category category;
    Level level;
    bool should_ratelimit{false}; //!< Hint for consumers if this entry should be ratelimited
    SourceLocation source_loc;
    std::string message;
};

/** Return whether messages with specified category and level should be logged. Applications using
 * the logging library need to provide this. */
bool ShouldLog(Logger* logger, Category category, Level level);

/** Send message to be logged. Applications using the logging library need to provide this. */
void Log(Logger* logger, Entry entry);

//! Object representing a context of log messages. Holds a logging category, an
//! optional log pointer which can be used by the application's log handler to
//! determine where to log to, and a Format hook to control message formatting.
struct Context {
    Category category;
    Logger* logger;

    explicit Context(Category category = BCLog::LogFlags::ALL, Logger* logger = nullptr) : category{category}, logger{logger} {}

    template <typename... Args>
    std::string Format(util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args) const
    {
        std::string log_msg;
        try {
            log_msg = tfm::format(fmt, args...);
        } catch (tinyformat::format_error& fmterr) {
            log_msg = "Error \"" + std::string{fmterr.what()} + "\" while formatting log message: " + fmt.fmt;
        }
        return log_msg;
    }
};

namespace detail {
//! Internal helper to get log context object from the first macro argument.
inline Context& GetContext(Context& context LIFETIMEBOUND) { return context; }
inline Context GetContext(Category category) { return Context{category}; }
inline Context GetContext(std::string_view fmt) { return Context{}; }

//! Internal helper to format log arguments and call a logging function.
//! Overloaded to detect case where first macro argument is a string literal and
//! context has been omitted.
template <typename Context, typename... Args>
void Log(Level level, bool should_ratelimit, SourceLocation&& source_loc, Context&& context, ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    Log(context.logger, Entry{
        .category = context.category,
        .level = level,
        .should_ratelimit = should_ratelimit,
        .source_loc = std::move(source_loc),
        .message = context.Format(fmt, args...)});
}
template <typename Context, typename ContextArg, typename... Args>
requires (!std::is_convertible_v<ContextArg, std::string_view>)
void Log(Level level, bool should_ratelimit, SourceLocation&& source_loc, Context&& context, ContextArg&&, ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    Log(level, should_ratelimit, std::move(source_loc), context, fmt, args...);
}
} // namespace detail
} // namespace util::log

//! Internal helper to return first arg in a __VA_ARGS__ pack.
#define FirstArg_(arg, ...) arg

//! Internal helper to conditionally log. Only evaluates arguments when needed.
// Allow __func__ to be used in any context without warnings:
// NOLINTBEGIN(bugprone-lambda-function-name)
#define LogPrint_(level, should_ratelimit, ...)                                                    \
    do {                                                                                           \
        auto&& _context{util::log::detail::GetContext(FirstArg_(__VA_ARGS__))};                    \
        if (util::log::ShouldLog(_context.logger, _context.category, (level))) {                   \
            util::log::detail::Log((level), (should_ratelimit), SourceLocation{__func__},          \
                                   _context, __VA_ARGS__);                                         \
        } else if ((level) >= util::log::Level::Info) {                                            \
            /* For Info levels and up, we guarantee that arguments are always evaluated. */        \
            [](auto&&...) {}(__VA_ARGS__);                                                         \
        }                                                                                          \
    } while (0)
// NOLINTEND(bugprone-lambda-function-name)

//! Logging macros which output log messages at the specified levels. The
//! macros accept an optional log context or category parameter followed by a
//! printf-style format string and arguments.
//!
//! If severity level is Info or higher, rate limiting is applied to mitigate
//! disk filling attacks. Users enabling logging at Debug and lower levels are
//! assumed to be developers or power users who are aware that -debug may cause
//! excessive disk usage due to logging.
#define LogError(...) LogPrint_(util::log::Level::Error, /*should_ratelimit=*/true, __VA_ARGS__)
#define LogWarning(...) LogPrint_(util::log::Level::Warning, /*should_ratelimit=*/true, __VA_ARGS__)
#define LogInfo(...) LogPrint_(util::log::Level::Info, /*should_ratelimit=*/true, __VA_ARGS__)
#define LogDebug(...) LogPrint_(util::log::Level::Debug, /*should_ratelimit=*/false, __VA_ARGS__)
#define LogTrace(...) LogPrint_(util::log::Level::Trace, /*should_ratelimit=*/false, __VA_ARGS__)
#define LogPrintLevel_(context, level, should_ratelimit, ...) LogPrint_((level), (should_ratelimit), (context), __VA_ARGS__)

namespace BCLog {
//! Alias for compatibility. Prefer util::log::Level over BCLog::Level in new code.
using Level = util::log::Level;
} // namespace BCLog

#endif // BITCOIN_UTIL_LOG_H
