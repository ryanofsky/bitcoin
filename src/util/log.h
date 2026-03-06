// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_LOG_H
#define BITCOIN_UTIL_LOG_H

#include <logging/categories.h> // IWYU pragma: export
#include <threadsafety.h>
#include <tinyformat.h>
#include <util/check.h>
#include <util/string.h>
#include <util/threadnames.h>

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
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
    std::string thread_name{util::ThreadGetInternalName()};
    SourceLocation source_loc;
    std::string message;
};

/** Return whether messages with specified category and level should be logged. Applications using
 * the logging library need to provide this. */
bool ShouldLog(Category category, Level level);

/** Send message to be logged. Applications using the logging library need to provide this. */
void Log(Entry entry);

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
     * @param[in] level             Severity level.
     * @param[in] category          Opaque category identifier.
     * @param[in] loc               Source location of the log call.
     * @param[in] should_ratelimit  Hint for consumers if this entry should be ratelimited.
     * @param[in] fmt               Format string.
     * @param[in] args              Format arguments.
     */
    template <typename... Args>
    void Log(Level level, uint64_t category, SourceLocation&& loc, bool should_ratelimit,
             util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        std::string log_msg;
        try {
            log_msg = tfm::format(fmt, args...);
        } catch (const tinyformat::format_error& fmterr) {
            log_msg = "Error \"" + std::string{fmterr.what()} + "\" while formatting log message: " + fmt.fmt;
        }

        Entry entry{
            .category = category,
            .level = level,
            .should_ratelimit = should_ratelimit,
            .source_loc = std::move(loc),
            .message = std::move(log_msg),
        };

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

#endif // BITCOIN_UTIL_LOG_H
