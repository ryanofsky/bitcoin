// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LOGGING_H
#define BITCOIN_LOGGING_H

#include <attributes.h>
#include <threadsafety.h>
#include <tinyformat.h>
#include <util/fs.h>
#include <util/string.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace BCLog {
class Logger;
} // namespace BCLog

BCLog::Logger& LogInstance();

static const bool DEFAULT_LOGTIMEMICROS = false;
static const bool DEFAULT_LOGIPS        = false;
static const bool DEFAULT_LOGTIMESTAMPS = true;
static const bool DEFAULT_LOGTHREADNAMES = false;
static const bool DEFAULT_LOGSOURCELOCATIONS = false;
static constexpr bool DEFAULT_LOGLEVELALWAYS = false;
extern const char * const DEFAULT_DEBUGLOGFILE;

extern bool fLogIPs;

struct LogCategory {
    std::string category;
    bool active;
};

namespace BCLog {
    enum LogFlags : uint32_t {
        NONE        = 0,
        NET         = (1 <<  0),
        TOR         = (1 <<  1),
        MEMPOOL     = (1 <<  2),
        HTTP        = (1 <<  3),
        BENCH       = (1 <<  4),
        ZMQ         = (1 <<  5),
        WALLETDB    = (1 <<  6),
        RPC         = (1 <<  7),
        ESTIMATEFEE = (1 <<  8),
        ADDRMAN     = (1 <<  9),
        SELECTCOINS = (1 << 10),
        REINDEX     = (1 << 11),
        CMPCTBLOCK  = (1 << 12),
        RAND        = (1 << 13),
        PRUNE       = (1 << 14),
        PROXY       = (1 << 15),
        MEMPOOLREJ  = (1 << 16),
        LIBEVENT    = (1 << 17),
        COINDB      = (1 << 18),
        QT          = (1 << 19),
        LEVELDB     = (1 << 20),
        VALIDATION  = (1 << 21),
        I2P         = (1 << 22),
        IPC         = (1 << 23),
#ifdef DEBUG_LOCKCONTENTION
        LOCK        = (1 << 24),
#endif
        UTIL        = (1 << 25),
        BLOCKSTORAGE = (1 << 26),
        TXRECONCILIATION = (1 << 27),
        SCAN        = (1 << 28),
        TXPACKAGES  = (1 << 29),
        ALL         = ~(uint32_t)0,
    };
    enum class Level {
        Trace = 0, // High-volume or detailed logging for development/debugging
        Debug,     // Reasonably noisy logging, but still usable in production
        Info,      // Default
        Warning,
        Error,
    };
    constexpr auto DEFAULT_LOG_LEVEL{Level::Debug};

    class Logger
    {
    private:
        mutable StdMutex m_cs; // Can not use Mutex from sync.h because in debug mode it would cause a deadlock when a potential deadlock was detected

        FILE* m_fileout GUARDED_BY(m_cs) = nullptr;
        std::list<std::string> m_msgs_before_open GUARDED_BY(m_cs);
        bool m_buffering GUARDED_BY(m_cs) = true; //!< Buffer messages before logging can be started.

        /**
         * m_started_new_line is a state variable that will suppress printing of
         * the timestamp when multiple calls are made that don't end in a
         * newline.
         */
        std::atomic_bool m_started_new_line{true};

        //! Category-specific log level. Overrides `m_log_level`.
        std::unordered_map<LogFlags, Level> m_category_log_levels GUARDED_BY(m_cs);

        //! If there is no category-specific log level, all logs with a severity
        //! level lower than `m_log_level` will be ignored.
        std::atomic<Level> m_log_level{DEFAULT_LOG_LEVEL};

        /** Log categories bitfield. */
        std::atomic<uint32_t> m_categories{0};

        std::string LogTimestampStr(const std::string& str);

        /** Slots that connect to the print signal */
        std::list<std::function<void(const std::string&)>> m_print_callbacks GUARDED_BY(m_cs) {};

    public:
        bool m_print_to_console = false;
        bool m_print_to_file = false;

        bool m_log_timestamps = DEFAULT_LOGTIMESTAMPS;
        bool m_log_time_micros = DEFAULT_LOGTIMEMICROS;
        bool m_log_threadnames = DEFAULT_LOGTHREADNAMES;
        bool m_log_sourcelocations = DEFAULT_LOGSOURCELOCATIONS;
        bool m_always_print_category_level = DEFAULT_LOGLEVELALWAYS;

        fs::path m_file_path;
        std::atomic<bool> m_reopen_file{false};

        std::string GetLogPrefix(LogFlags category, Level level) const;

        /** Send a string to the log output */
        void LogPrintStr(const std::string& str, const std::string& logging_function, const std::string& source_file, int source_line, BCLog::LogFlags category, BCLog::Level level);

        /** Returns whether logs will be written to any output */
        bool Enabled() const
        {
            StdLockGuard scoped_lock(m_cs);
            return m_buffering || m_print_to_console || m_print_to_file || !m_print_callbacks.empty();
        }

        /** Connect a slot to the print signal and return the connection */
        std::list<std::function<void(const std::string&)>>::iterator PushBackCallback(std::function<void(const std::string&)> fun)
        {
            StdLockGuard scoped_lock(m_cs);
            m_print_callbacks.push_back(std::move(fun));
            return --m_print_callbacks.end();
        }

        /** Delete a connection */
        void DeleteCallback(std::list<std::function<void(const std::string&)>>::iterator it)
        {
            StdLockGuard scoped_lock(m_cs);
            m_print_callbacks.erase(it);
        }

        /** Start logging (and flush all buffered messages) */
        bool StartLogging();
        /** Only for testing */
        void DisconnectTestLogger();

        void ShrinkDebugFile();

        std::unordered_map<LogFlags, Level> CategoryLevels() const
        {
            StdLockGuard scoped_lock(m_cs);
            return m_category_log_levels;
        }
        void SetCategoryLogLevel(const std::unordered_map<LogFlags, Level>& levels)
        {
            StdLockGuard scoped_lock(m_cs);
            m_category_log_levels = levels;
        }
        bool SetCategoryLogLevel(const std::string& category_str, const std::string& level_str);

        Level LogLevel() const { return m_log_level.load(); }
        void SetLogLevel(Level level) { m_log_level = level; }
        bool SetLogLevel(const std::string& level);

        uint32_t GetCategoryMask() const { return m_categories.load(); }

        void EnableCategory(LogFlags flag);
        bool EnableCategory(const std::string& str);
        void DisableCategory(LogFlags flag);
        bool DisableCategory(const std::string& str);

        bool WillLogCategory(LogFlags category) const;
        bool WillLogCategoryLevel(LogFlags category, Level level) const;

        /** Returns a vector of the log categories in alphabetical order. */
        std::vector<LogCategory> LogCategoriesList() const;
        /** Returns a string with the log categories in alphabetical order. */
        std::string LogCategoriesString() const
        {
            return Join(LogCategoriesList(), ", ", [&](const LogCategory& i) { return i.category; });
        };

        //! Returns a string with all user-selectable log levels.
        std::string LogLevelsString() const;

        //! Returns the string representation of a log level.
        static std::string LogLevelToStr(BCLog::Level level);

        bool DefaultShrinkDebugFile() const;
    };

    //! Object representing a particular source of log messages. Holds a logging
    //! category, a reference to the logger object to output to, and a
    //! formatting hook.
    struct Source {
        static constexpr bool log_source{true};
        LogFlags category;
        Logger& logger;

        Source(LogFlags category = LogFlags::ALL, Logger& logger = LogInstance()) : category(category), logger(logger) {}

        template <typename... Args>
        std::string Format(const char* fmt, const Args&... args) const
        {
            std::string log_msg;
            try {
                log_msg = tfm::format(fmt, args...);
            } catch (tinyformat::format_error& fmterr) {
                /* Original format string will have newline so don't add one here */
                log_msg = "Error \"" + std::string(fmterr.what()) + "\" while formatting log message: " + fmt;
            }
            return log_msg;
        }
    };

} // namespace BCLog

/** Return true if log accepts specified category, at the specified level. */
static inline bool LogAcceptCategory(BCLog::LogFlags category, BCLog::Level level)
{
    return LogInstance().WillLogCategoryLevel(category, level);
}

//! Determine whether logging is enabled from source at a logging level.
template<typename Source>
static inline bool LogAccept(const Source& source, BCLog::Level level)
{
    return source.logger.WillLogCategoryLevel(source.category, level);
}

/** Return true if str parses as a log category and set the flag */
bool GetLogCategory(BCLog::LogFlags& flag, const std::string& str);

// Be conservative when using LogPrintf/error or other things which
// unconditionally log to debug.log! It should not be the case that an inbound
// peer can fill up a user's disk with debug.log entries.

//! Internal helper. Implicitly convert macro source argument to BCLog::Source reference.
static inline const BCLog::Source& _LogSource(const BCLog::Source& source LIFETIMEBOUND) { return source; }

//! Internal helper. Overload for _LogSource accepting custom sources that may override the Format method.
template <typename Source>
static inline std::enable_if_t<Source::log_source, const Source&> _LogSource(const Source& source LIFETIMEBOUND) { return source; }

//! Internal helper. Format logging arguments and log.
template <typename Source, typename... Args>
static inline void _LogArgs(const Source& source, const std::string& logging_function, const std::string& source_file, const int source_line, const BCLog::Level level, const char* fmt, const Args&... args)
{
    if (source.logger.Enabled()) {
        source.logger.LogPrintStr(source.Format(fmt, args...), logging_function, source_file, source_line, source.category, level);
    }
}

//! Internal helper. Attach logging location and log.
#define _LogLocation(source, level, ...) _LogArgs(source, __func__, __FILE__, __LINE__, level, __VA_ARGS__)

//! Internal helper. Check logging category and log. Avoid evaluating arguments if not logging.
#define _LogCategory(source, level, ...)               \
    do {                                                  \
        if (LogAccept(_LogSource(source), (level))) {     \
            _LogLocation(_LogSource(source), (level), __VA_ARGS__); \
        }                                                 \
    } while (0)

//! Logging macros output log messages at the specified levels, and avoid
//! evaluating their arguments if logging is not enabled for the level. The
//! macros accept a BCLog::Source parameter followed by a printf-style format
//! string and arguments.
//
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
//! Log sources can be hardcoded categories like:
//!
//!   LogDebug(BCLog::TXRECONCILIATION, "Forget txreconciliation state of peer=%d\n", peer_id);
//!
//! Or log source objects can be defined to reduce repetition and verbosity:
//!
//!   const BCLog::Source m_log{BCLog::TXRECONCILIATION};
//!   ...
//!   LogDebug(m_log, "Forget txreconciliation state of peer=%d\n", peer_id);
//!
//! Using source objects also provides the flexibility to add extra information
//! and custom formatting to log messages, or to divert log messages to a local
//! logger instead of the global logging instance, without needing to change
//! existing log statements.
#define LogError(source, ...) _LogCategory(source, BCLog::Level::Error, __VA_ARGS__)
#define LogWarning(source, ...) _LogCategory(source, BCLog::Level::Warning, __VA_ARGS__)
#define LogInfo(source, ...) _LogCategory(source, BCLog::Level::Info, __VA_ARGS__)
#define LogDebug(source, ...) _LogCategory(source, BCLog::Level::Debug, __VA_ARGS__)
#define LogTrace(source, ...) _LogCategory(source, BCLog::Level::Trace, __VA_ARGS__)
#define LogPrintLevel(source, level, ...) _LogCategory(source, level, __VA_ARGS__)

//! Deprecated functions relying on global variable. Avoid these and use BCLog::Source in new code.
#define LogPrint(category, ...) LogDebug({(category)}, __VA_ARGS__)
#define LogPrintf(...) LogInfo({}, __VA_ARGS__)

template <typename... Args>
bool error(const char* fmt, const Args&... args)
{
    LogPrintf("ERROR: %s\n", tfm::format(fmt, args...));
    return false;
}

#endif // BITCOIN_LOGGING_H
