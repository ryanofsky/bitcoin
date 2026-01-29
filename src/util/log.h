// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_LOG_H
#define BITCOIN_UTIL_LOG_H

// This header works in tandem with `logging/categories.h`
// to expose the complete logging interface.
#include <logging/categories.h> // IWYU pragma: export
#include <threadsafety.h>
#include <tinyformat.h>
#include <util/check.h>
<<<<<<< HEAD
#include <util/threadnames.h>
#include <util/time.h>
||||||| parent of 6c148e8c405 (util: add log::Dispatcher for struct-based log dispatch)
=======
#include <util/stdmutex.h>
#include <util/string.h>
#include <util/threadnames.h>
>>>>>>> 6c148e8c405 (util: add log::Dispatcher for struct-based log dispatch)

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <list>
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
<<<<<<< HEAD
    SystemClock::time_point timestamp{SystemClock::now()};
    std::chrono::seconds mocktime{GetMockTime()};
    std::string thread_name{util::ThreadGetInternalName()};
||||||| parent of 6c148e8c405 (util: add log::Dispatcher for struct-based log dispatch)
=======
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
    std::string thread_name{util::ThreadGetInternalName()};
>>>>>>> 6c148e8c405 (util: add log::Dispatcher for struct-based log dispatch)
    SourceLocation source_loc;
    std::string message;
};

/// Return whether messages with specified category should be debug logged.
/// Applications using the logging library need to provide this.
bool ShouldDebugLog(Category category);

/// Return whether messages with specified category should be trace logged.
/// Applications using the logging library need to provide this.
bool ShouldTraceLog(Category category);

/** Send message to be logged. Applications using the logging library need to provide this. */
void Log(Entry entry);
<<<<<<< HEAD
||||||| parent of 6c148e8c405 (util: add log::Dispatcher for struct-based log dispatch)
} // namespace util::log

namespace BCLog {
//! Alias for compatibility. Prefer util::log::Level over BCLog::Level in new code.
using Level = util::log::Level;
} // namespace BCLog
=======

/**
 * Dispatcher is responsible for producing logs. It forwards log entries to one or
 * multiple logging sinks (e.g. BCLog::Logger) through its registered callbacks.
 *
 * Log consumption (including printing and rate limiting) should be implemented by the sink.
 */
class Dispatcher
{
public:
    //! Type for callbacks invoked for each log entry.
    using Callback = std::function<void(const Entry&)>;
    //! Type for opaque handles returned by RegisterCallback(), used to unregister.
    using CallbackHandle = std::list<Callback>::iterator;

    /**
     * Register a callback to receive log entries.
     * @param[in] callback  Invoked for each log entry.
     * @return Handle to use with UnregisterCallback().
     */
    [[nodiscard]] CallbackHandle RegisterCallback(Callback callback) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Unregister a previously registered callback.
     * @param[in] handle  Handle previously returned by RegisterCallback().
     */
    void UnregisterCallback(CallbackHandle handle) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** @return true if any callbacks are registered. */
    bool Enabled() const { return m_callback_count.load(std::memory_order_acquire) > 0; }

    /**
     * Format message and dispatch to all registered callbacks.
     */
    void Log(const Entry& entry) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        StdLockGuard lock{m_mutex};
        for (const auto& callback : m_callbacks) {
            callback(entry);
        }
    }

private:
    mutable StdMutex m_mutex;
    //! Callbacks to be executed when an eligible log statement is produced.
    std::list<Callback> m_callbacks GUARDED_BY(m_mutex);
    //! Lock-free size of m_callbacks for fast checks.
    std::atomic<size_t> m_callback_count{0};
};
} // namespace util::log

namespace BCLog {
//! Alias for compatibility. Prefer util::log::Level over BCLog::Level in new code.
using Level = util::log::Level;
} // namespace BCLog
>>>>>>> 6c148e8c405 (util: add log::Dispatcher for struct-based log dispatch)

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

#endif // BITCOIN_UTIL_LOG_H
