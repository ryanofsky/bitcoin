// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_LOG_H
#define BITCOIN_UTIL_LOG_H

// This header works in tandem with `logging/categories.h`
// to expose the complete logging interface.
#include <attributes.h>
#include <logging/categories.h> // IWYU pragma: export
#include <tinyformat.h>
#include <util/check.h>
#include <util/threadnames.h>
#include <util/time.h>

#include <cstdint>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>

//! Logging macros which output log messages at the specified levels. They
//! accept printf-style format strings and arguments. The debug and trace macros
//! also require an initial BCLog::LogFlags category argument.
//! See the "Logging" section of /doc/developer-notes.md for more details.
#define LogError(...) LOG_EMIT((.level = util::log::Level::Error), __VA_ARGS__)
#define LogWarning(...) LOG_EMIT((.level = util::log::Level::Warning), __VA_ARGS__)
#define LogInfo(...) LOG_EMIT((.level = util::log::Level::Info), __VA_ARGS__)
#define LogDebug(...) LOG_EMIT((.level = util::log::Level::Debug), __VA_ARGS__)
#define LogTrace(...) LOG_EMIT((.level = util::log::Level::Trace), __VA_ARGS__)

//! Low-level logging macro called with an initial options argument. Meant to be
//! called internally, and in special cases to override default behaviors.
// NOLINTBEGIN(bugprone-lambda-function-name)
// Allow __func__ to be used in any context without warnings:
#define LOG_EMIT(options, ...)                                                                     \
    do {                                                                                           \
        constexpr util::log::Options _options{PP_EXPAND_ARGS options};                             \
        auto&& _context{util::log::detail::GetContext<_options>(PP_FIRST_ARG(__VA_ARGS__))};       \
        if (util::log::ShouldLog(_context.category, _options.level)) {                             \
            util::log::detail::Emit(_options, SourceLocation{__func__}, _context, __VA_ARGS__);    \
        } else if constexpr (_options.always_evaluate_arguments) {                                 \
            [](auto&&...) {}(__VA_ARGS__);                                                         \
        }                                                                                          \
    } while (0)
// NOLINTEND(bugprone-lambda-function-name)

//! Return the first argument from a variadic macro argument list.
#define PP_FIRST_ARG(arg, ...) arg

//! Expand parenthesized macro arguments `(a, b)` into `a, b`.
#define PP_EXPAND_ARGS(...) __VA_ARGS__

/// Like std::source_location, but allowing to override the function name.
class SourceLocation
{
public:
    /// The func argument must be constructed from the C++11 __func__ macro.
    /// Ref: https://en.cppreference.com/w/cpp/language/function.html#func
    /// Non-static string literals are not supported.
    explicit SourceLocation(
        const char* func,
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

//! Structure and constant for tagging not to rate limit.
struct NoRateLimitTag {
    explicit NoRateLimitTag() = default;
};
inline constexpr NoRateLimitTag NO_RATE_LIMIT{};

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
    SystemClock::time_point timestamp{SystemClock::now()};
    std::chrono::seconds mocktime{GetMockTime()};
    std::string thread_name{util::ThreadGetInternalName()};
    SourceLocation source_loc;
    std::string message;
};

<<<<<<< HEAD
/// Return whether messages with specified category should be debug logged.
/// Applications using the logging library need to provide this.
bool ShouldDebugLog(Category category);

/// Return whether messages with specified category should be trace logged.
/// Applications using the logging library need to provide this.
bool ShouldTraceLog(Category category);
||||||| parent of af3ca8272d2 (log refactor: log macro rewrite)
/** Return whether messages with specified category and level should be logged. Applications using
 * the logging library need to provide this. */
bool ShouldLog(Category category, Level level);
=======
//! Options that can be passed to log macros using designated initializers (e.g.
//! `.ratelimit = true`). Add new options here when introducing new logging
//! behaviors.
struct Options {
    Level level;

    // Info and higher log messages are logged by default, so rate limit them.
    // Users enabling logging at Debug and lower levels are assumed to be
    // developers or power users who are aware that -debug may cause excessive
    // disk usage due to logging.
    bool ratelimit{level >= Level::Info};

    // Always evaluate format arguments at Info and higher levels because logs
    // at these levels are rarely disabled, so it's safer to evaluate unused
    // arguments than risk unintended side effects when logging is disabled.
    bool always_evaluate_arguments{level >= Level::Info};
};

//! Object representing context of log messages. Holds a logging category and
//! implements log formatting.
struct Context {
    Category category;

    explicit Context(Category category = BCLog::LogFlags::ALL) : category{category} {}

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

/** Return whether messages with specified category and level should be logged. Applications using
 * the logging library need to provide this. */
bool ShouldLog(Category category, Level level);
>>>>>>> af3ca8272d2 (log refactor: log macro rewrite)

/** Send message to be logged. Applications using the logging library need to provide this. */
void Log(Entry entry);
<<<<<<< HEAD
||||||| parent of af3ca8272d2 (log refactor: log macro rewrite)
} // namespace util::log

namespace BCLog {
//! Alias for compatibility. Prefer util::log::Level over BCLog::Level in new code.
using Level = util::log::Level;
} // namespace BCLog
=======

namespace detail {
//! Internal helper to get Context from the first macro argument. Overloaded to
//! detect case where first macro argument is a string literal instead of a
//! category.
template <Options options>
Context GetContext(std::string_view fmt)
{
    // Trigger compile error if caller does not pass a category constant as the
    // first argument to a logging call, unless the level is Info or higher.
    // There is no technical reason category arguments must be required, but
    // categories are useful for finding and filtering relevant messages when
    // debugging, so we want to encourage them.
    static_assert(options.level >= Level::Info, "Missing required category argument for Debug/Trace logging call. Category can only be omitted for Info/Warning/Error calls.");
    return Context{};
}
template <Options options>
Context GetContext(Category category)
{
    // Trigger compile error if caller tries to pass a category constant as a
    // first argument to a logging call at Info level or higher. There is no
    // technical reason why all logging calls could not accept category
    // arguments, but for various reasons, such as (1) not wanting to allow
    // users to filter by category at high priority levels, and (2) wanting to
    // incentivize developers to use lower log levels to avoid log spam, passing
    // category constants at higher levels is forbidden.
    static_assert(options.level < Level::Info, "Cannot pass category argument to Info/Warning/Error logging call. Please switch to Debug/Trace call, or drop the category argument!");
    return Context{category};
}

//! Internal helper to construct log entry and emit log message. Overloaded to
//! detect case where first macro argument is a string literal instead of a
//! category.
template <typename Context, typename... Args>
void Emit(Options options, SourceLocation&& source_loc, Context&& context, ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    Log(Entry{
        .category = context.category,
        .level = options.level,
        .should_ratelimit = options.ratelimit,
        .source_loc = std::move(source_loc),
        .message = context.Format(fmt, args...)});
}
template <typename Context, typename ContextArg, typename... Args>
requires (!std::is_convertible_v<ContextArg, std::string_view>)
void Emit(Options options, SourceLocation&& source_loc, Context&& context, ContextArg&&, ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    Emit(std::move(options), std::move(source_loc), context, fmt, args...);
}
} // namespace detail
} // namespace util::log

namespace BCLog {
//! Alias for compatibility. Prefer util::log::Level over BCLog::Level in new code.
using Level = util::log::Level;
} // namespace BCLog
>>>>>>> af3ca8272d2 (log refactor: log macro rewrite)

<<<<<<< HEAD
template <typename... Args>
inline void LogPrintFormatInternal_(SourceLocation&& source_loc, BCLog::LogFlags flag, util::log::Level level, bool should_ratelimit, util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    std::string log_msg;
    try {
        log_msg = tfm::format(fmt, args...);
    } catch (tinyformat::format_error& fmterr) {
        log_msg = "Error \"" + std::string{fmterr.what()} + "\" while formatting log message: " + fmt.fmt;
    }
    util::log::Log(util::log::Entry{
        .category = flag,
        .level = level,
        .should_ratelimit = should_ratelimit,
        .source_loc = std::move(source_loc),
        .message = std::move(log_msg)});
}

template <typename... Args>
inline void LogPrintFormatInternal(SourceLocation&& source_loc, BCLog::LogFlags flag, util::log::Level level, util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    return LogPrintFormatInternal_(std::move(source_loc), flag, level, /*should_ratelimit=*/true, fmt, args...);
}

template <typename... Args>
inline void LogPrintFormatInternal(SourceLocation&& source_loc, BCLog::LogFlags flag, util::log::Level level, util::log::NoRateLimitTag, util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    return LogPrintFormatInternal_(std::move(source_loc), flag, level, /*should_ratelimit=*/false, fmt, args...);
}
} // namespace util::log

namespace BCLog {
//! Alias for compatibility. Prefer util::log::Level over BCLog::Level in new code.
using Level = util::log::Level;
} // namespace BCLog

// Allow __func__ to be used in any context without warnings:
// NOLINTNEXTLINE(bugprone-lambda-function-name)
#define detail_LogWithSrcLoc(category, level, ...) util::log::LogPrintFormatInternal(SourceLocation{__func__}, category, level, __VA_ARGS__)

// Log unconditionally. Uses basic rate limiting to mitigate disk filling attacks.
// Be conservative when using functions that unconditionally log to debug.log!
// It should not be the case that an inbound peer can fill up a user's storage
// with debug.log entries.
#define LogInfo(...) detail_LogWithSrcLoc(BCLog::LogFlags::ALL, util::log::Level::Info, __VA_ARGS__)
#define LogWarning(...) detail_LogWithSrcLoc(BCLog::LogFlags::ALL, util::log::Level::Warning, __VA_ARGS__)
#define LogError(...) detail_LogWithSrcLoc(BCLog::LogFlags::ALL, util::log::Level::Error, __VA_ARGS__)

// Use a macro instead of a function for conditional logging to prevent
// evaluating arguments when logging for the category is not enabled.

// Log by prefixing the output with the passed category name and severity level. This logs conditionally if
// the category is allowed. No rate limiting is applied, because users specifying -debug are assumed to be
// developers or power users who are aware that -debug may cause excessive disk usage due to logging.
#define detail_LogIfCategoryAndLevelEnabled(category, shouldlog, level, ...)                  \
    do {                                                                                      \
        if (shouldlog(category)) {                                                            \
            detail_LogWithSrcLoc((category), (level), util::log::NO_RATE_LIMIT, __VA_ARGS__); \
        }                                                                                     \
    } while (0)

// Log conditionally, prefixing the output with the passed category name.
#define LogDebug(category, ...) detail_LogIfCategoryAndLevelEnabled(category, util::log::ShouldDebugLog, util::log::Level::Debug, __VA_ARGS__)
#define LogTrace(category, ...) detail_LogIfCategoryAndLevelEnabled(category, util::log::ShouldTraceLog, util::log::Level::Trace, __VA_ARGS__)

||||||| parent of af3ca8272d2 (log refactor: log macro rewrite)
template <typename... Args>
inline void LogPrintFormatInternal(SourceLocation&& source_loc, BCLog::LogFlags flag, BCLog::Level level, bool should_ratelimit, util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    std::string log_msg;
    try {
        log_msg = tfm::format(fmt, args...);
    } catch (tinyformat::format_error& fmterr) {
        log_msg = "Error \"" + std::string{fmterr.what()} + "\" while formatting log message: " + fmt.fmt;
    }
    util::log::Log(util::log::Entry{
        .category = flag,
        .level = level,
        .should_ratelimit = should_ratelimit,
        .source_loc = std::move(source_loc),
        .message = std::move(log_msg)});
}

// Allow __func__ to be used in any context without warnings:
// NOLINTNEXTLINE(bugprone-lambda-function-name)
#define LogPrintLevel_(category, level, should_ratelimit, ...) LogPrintFormatInternal(SourceLocation{__func__}, category, level, should_ratelimit, __VA_ARGS__)

// Log unconditionally. Uses basic rate limiting to mitigate disk filling attacks.
// Be conservative when using functions that unconditionally log to debug.log!
// It should not be the case that an inbound peer can fill up a user's storage
// with debug.log entries.
#define LogInfo(...) LogPrintLevel_(BCLog::LogFlags::ALL, BCLog::Level::Info, /*should_ratelimit=*/true, __VA_ARGS__)
#define LogWarning(...) LogPrintLevel_(BCLog::LogFlags::ALL, BCLog::Level::Warning, /*should_ratelimit=*/true, __VA_ARGS__)
#define LogError(...) LogPrintLevel_(BCLog::LogFlags::ALL, BCLog::Level::Error, /*should_ratelimit=*/true, __VA_ARGS__)

// Use a macro instead of a function for conditional logging to prevent
// evaluating arguments when logging for the category is not enabled.

// Log by prefixing the output with the passed category name and severity level. This logs conditionally if
// the category is allowed. No rate limiting is applied, because users specifying -debug are assumed to be
// developers or power users who are aware that -debug may cause excessive disk usage due to logging.
#define detail_LogIfCategoryAndLevelEnabled(category, level, ...)      \
    do {                                                               \
        if (util::log::ShouldLog((category), (level))) {               \
            bool rate_limit{level >= BCLog::Level::Info};              \
            Assume(!rate_limit); /*Only called with the levels below*/ \
            LogPrintLevel_(category, level, rate_limit, __VA_ARGS__);  \
        }                                                              \
    } while (0)

// Log conditionally, prefixing the output with the passed category name.
#define LogDebug(category, ...) detail_LogIfCategoryAndLevelEnabled(category, BCLog::Level::Debug, __VA_ARGS__)
#define LogTrace(category, ...) detail_LogIfCategoryAndLevelEnabled(category, BCLog::Level::Trace, __VA_ARGS__)

=======
>>>>>>> af3ca8272d2 (log refactor: log macro rewrite)
/** Return true if log accepts specified category, at the specified level. */
static inline bool LogAcceptCategory(BCLog::LogFlags category, BCLog::Level level)
{
    return util::log::ShouldLog(category, level);
}

#endif // BITCOIN_UTIL_LOG_H
