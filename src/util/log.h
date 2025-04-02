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

//! Log level constants. Most code will not need to use these directly and can
//! use LogTrace, LogDebug, LogInfo, LogWarning, and LogError macros defined
//! below. See macro definitions below or "Logging" section in
//! developer-notes.md for more detailed information.
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

//! Object representing a source of log messages. Holds a logging category, an
//! optional log pointer which can be used by the application's log handler to
//! determine where to log to, and a Format hook to control message formatting.
struct Source {
    static constexpr bool log_source{true};
    Category category;
    Logger* logger;

    explicit Source(Category category = BCLog::LogFlags::ALL, Logger* logger = nullptr) : category{category}, logger{logger} {}

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
//! Internal helper to get log source object from the first macro argument.
template <typename Source>
requires (Source::log_source)
const Source& GetSource(const Source& source LIFETIMEBOUND) { return source; }
inline Source GetSource(Category category) { return Source{category}; }
inline Source GetSource(std::string_view fmt) { return Source{}; }

//! Internal helper to format log arguments and call a logging function.
//! Overloaded to detect case where first macro argument is a string literal and
//! source has been omitted.
template <typename Source, typename... Args>
void Log(Level level, bool ratelimit, SourceLocation&& source_loc, Source&& source, ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    Log(source.logger, Entry{
        .category = source.category,
        .level = level,
        .should_ratelimit = ratelimit,
        .source_loc = std::move(source_loc),
        .message = source.Format(fmt, args...)});
}
template <typename Source, typename SourceArg, typename... Args>
requires (!std::is_convertible_v<SourceArg, std::string_view>)
void Log(Level level, bool ratelimit, SourceLocation&& source_loc, Source&& source, SourceArg&&, ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    Log(level, ratelimit, std::move(source_loc), source, fmt, args...);
}
} // namespace detail
} // namespace util::log

//! Internal helper to return first arg in a __VA_ARGS__ pack.
#define FirstArg_(arg, ...) arg

//! Internal helper to conditionally log. Only evaluates arguments when needed.
// Allow __func__ to be used in any context without warnings:
// NOLINTBEGIN(bugprone-lambda-function-name)
#define LogPrint_(level, ratelimit, ...)                                                           \
    do {                                                                                           \
        const auto& _source{util::log::detail::GetSource(FirstArg_(__VA_ARGS__))};                 \
        if (util::log::ShouldLog(_source.logger, _source.category, (level))) {                     \
            SourceLocation loc{SourceLocation{__func__}};                                          \
            util::log::detail::Log((level), ratelimit, std::move(loc), _source, __VA_ARGS__);      \
        } else if ((level) >= util::log::Level::Info) {                                            \
            /* For Info levels and up, we guarantee that arguments are always evaluated. */        \
            [](auto&&...) {}(__VA_ARGS__);                                                         \
        }                                                                                          \
    } while (0)
// NOLINTEND(bugprone-lambda-function-name)

//! Logging macros which output log messages at the specified levels, and avoid
//! evaluating their arguments if logging is not enabled for the level. The
//! macros accept an optional log source parameter followed by a printf-style
//! format string and arguments.
//!
//! - LogError(), LogWarning(), and LogInfo() are all enabled by default, so
//!   they should be called infrequently, in cases where they will not spam the
//!   log and take up disk space.
//!
//! - LogDebug() is enabled when debug logging is enabled, and should be used to
//!   show messages that can help users troubleshoot issues.
//!
//! - LogTrace() is enabled when both debug logging AND tracing are enabled, and
//!   should be used for fine-grained traces that will be helpful to developers.
//!
//! For more information about log levels, see the -debug and -loglevel
//! documentation, or the "Logging" section of developer notes.
//!
//! `LogDebug` and `LogTrace` macros should take an initial category argument,
//! so messages can be filtered by category, but categories can be omitted at
//! higher levels:
//!
//!   LogDebug(BCLog::TXRECONCILIATION, "Forget txreconciliation state of peer=%d\n", peer_id);
//!   LogInfo("Important information, no category.\n");
//!
//! Source arguments can also be passed to control log output (see class definition).
//!
//!   const util::log::Source m_log{BCLog::TXRECONCILIATION};
//!   ...
//!   LogDebug(m_log, "Forget txreconciliation state of peer=%d\n", peer_id);
//!
//! Using source objects also provides the flexibility to add extra information
//! and custom formatting to log messages, or to divert log messages to a local
//! logger instead of the global logging instance.
//!
//! If severity level is Info or higher, rate limiting is applied to mitigate
//! disk filling attacks. Users enabling logging at Debug and lower levels are
//! assumed to be developers or power users who are aware that -debug may cause
//! excessive disk usage due to logging.
#define LogError(...) LogPrint_(util::log::Level::Error, true, __VA_ARGS__)
#define LogWarning(...) LogPrint_(util::log::Level::Warning, true, __VA_ARGS__)
#define LogInfo(...) LogPrint_(util::log::Level::Info, true, __VA_ARGS__)
#define LogDebug(...) LogPrint_(util::log::Level::Debug, false, __VA_ARGS__)
#define LogTrace(...) LogPrint_(util::log::Level::Trace, false, __VA_ARGS__)
#define LogPrintLevel_(source, level, ratelimit, ...) LogPrint_((level), (ratelimit), (source), __VA_ARGS__)

namespace BCLog {
//! Alias for compatibility. Prefer util::log::Level over BCLog::Level in new code.
using Level = util::log::Level;
} // namespace BCLog

#endif // BITCOIN_UTIL_LOG_H
