// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/sqlite.h>

#include <logging.h>
#include <sync.h>
#include <util/memory.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/translation.h>
#include <wallet/db.h>

#include <sqlite3.h>
#include <stdint.h>
#include <unordered_set>

namespace {
    Mutex g_sqlite_mutex;
    //! Set of wallet file paths in use
    std::unordered_set<std::string> g_file_paths GUARDED_BY(g_sqlite_mutex);
} // namespace

static const char* DATABASE_FILENAME = "wallet.sqlite";

static void ErrorLogCallback(void* arg, int code, const char* msg)
{
    assert(arg == nullptr); // That's what we tell it to do during the setup
    LogPrintf("SQLite Error. Code: %d. Message: %s\n", code, msg);
}

SQLiteDatabase::SQLiteDatabase(const fs::path& dir_path, const fs::path& file_path, bool mock) :
    WalletDatabase(), m_mock(mock), m_dir_path(dir_path.string()), m_file_path(file_path.string())
{
    LogPrintf("Using SQLite Version %s\n", SQLiteDatabaseVersion());
    LogPrintf("Using wallet %s\n", m_dir_path);

    LOCK(g_sqlite_mutex);
    if (g_file_paths.empty()) {
        // Setup logging
        int ret = sqlite3_config(SQLITE_CONFIG_LOG, ErrorLogCallback, nullptr);
        if (ret != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to setup error log: %s\n", sqlite3_errstr(ret)));
        }
    }
    int ret = sqlite3_initialize(); // This is a no-op if sqlite3 is already initialized
    if (ret != SQLITE_OK) {
        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to initialize SQLite: %s\n", sqlite3_errstr(ret)));
    }
    assert(m_file_path.empty() || (!m_file_path.empty() && g_file_paths.count(m_file_path) == 0));
    g_file_paths.insert(m_file_path);
}

bool SQLiteDatabase::PrepareDirectory() const
{
    if (m_mock) return true;
    // Try to create the directory containing the wallet file and lock it
    TryCreateDirectories(m_dir_path);
    if (!LockDirectory(m_dir_path, ".walletlock")) {
        LogPrintf("Cannot obtain a lock on wallet directory %s. Another instance of bitcoin may be using it.\n", m_dir_path);
        return false;
    }
    return true;
}

void SQLiteDatabase::SetupSQLStatements()
{
    std::string read_sql = "SELECT value FROM main WHERE key = ?";
    std::string insert_sql = "INSERT INTO main VALUES(?, ?)";
    std::string overwrite_sql = "INSERT OR REPLACE INTO main VALUES(?, ?)";
    std::string delete_sql = "DELETE FROM main WHERE key = ?";
    std::string cursor_sql = "SELECT key, value FROM main";

    int res;
    if (!m_read_stmt) {
        if ((res = sqlite3_prepare_v2(m_db, read_sql.c_str(), -1, &m_read_stmt, nullptr)) != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to setup SQL statements: %s\n", sqlite3_errstr(res)));
        }
    }
    if (!m_insert_stmt) {
        if ((res = sqlite3_prepare_v2(m_db, insert_sql.c_str(), -1, &m_insert_stmt, nullptr)) != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to setup SQL statements: %s\n", sqlite3_errstr(res)));
        }
    }
    if (!m_overwrite_stmt) {
        if ((res = sqlite3_prepare_v2(m_db, overwrite_sql.c_str(), -1, &m_overwrite_stmt, nullptr)) != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to setup SQL statements: %s\n", sqlite3_errstr(res)));
        }
    }
    if (!m_delete_stmt) {
        if ((res = sqlite3_prepare_v2(m_db, delete_sql.c_str(), -1, &m_delete_stmt, nullptr)) != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to setup SQL statements: %s\n", sqlite3_errstr(res)));
        }
    }
    if (!m_cursor_stmt) {
        if ((res = sqlite3_prepare_v2(m_db, cursor_sql.c_str(), -1, &m_cursor_stmt, nullptr)) != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to setup SQL statements : %s\n", sqlite3_errstr(res)));
        }
    }
}

SQLiteDatabase::~SQLiteDatabase()
{
    Close();
    LOCK(g_sqlite_mutex);
    g_file_paths.erase(m_file_path);
    if (g_file_paths.empty()) {
        int ret = sqlite3_shutdown();
        if (ret != SQLITE_OK) {
            LogPrintf("SQLiteDatabase: Failed to shutdown SQLite: %s\n", sqlite3_errstr(ret));
        }
    }
}

void SQLiteDatabase::Open(const char* mode)
{
    if (!PrepareDirectory()) {
        throw std::runtime_error("Cannot obtain a lock on wallet directory");
    }

    bool read_only = (!strchr(mode, '+') && !strchr(mode, 'w'));

    bool create = strchr(mode, 'c') != nullptr;
    int flags;
    if (read_only) {
        flags = SQLITE_OPEN_READONLY;
    } else {
        flags = SQLITE_OPEN_READWRITE;
    }
    if (create) {
        flags |= SQLITE_OPEN_CREATE;
    }
    if (m_mock) {
        flags |= SQLITE_OPEN_MEMORY; // In memory database for mock db
    }

    if (m_db == nullptr) {
        sqlite3* db = nullptr;
        int ret = sqlite3_open_v2(m_file_path.c_str(), &db, flags, nullptr);
        if (ret != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to open database: %s\n", sqlite3_errstr(ret)));
        }
        // TODO: Maybe(?) Check the file wasn't copied and a duplicate opened

        if (create) {
            bool table_exists;
            // Check that the main table exists
            sqlite3_stmt* check_main_stmt;
            std::string check_main = "SELECT name FROM sqlite_master WHERE type='table' AND name='main'";
            ret = sqlite3_prepare_v2(db, check_main.c_str(), -1, &check_main_stmt, nullptr);
            if (ret != SQLITE_OK) {
                throw std::runtime_error(strprintf("SQLiteDatabase: Failed to prepare statement to check table existence: %s\n", sqlite3_errstr(ret)));
            }
            ret = sqlite3_step(check_main_stmt);
            if (sqlite3_finalize(check_main_stmt) != SQLITE_OK) {
                throw std::runtime_error(strprintf("SQLiteDatabase: Failed to finalize statement checking table existence: %s\n", sqlite3_errstr(ret)));
            }
            if (ret == SQLITE_DONE) {
                table_exists = false;
            } else if (ret == SQLITE_ROW) {
                table_exists = true;
            } else {
                throw std::runtime_error(strprintf("SQLiteDatabase: Failed to execute statement to check table existence: %s\n", sqlite3_errstr(ret)));
            }

            if (!table_exists) {
                // Make the table for our key-value pairs
                std::string create_stmt = "CREATE TABLE main(key BLOB PRIMARY KEY, value BLOB)";
                ret = sqlite3_exec(db, create_stmt.c_str(), nullptr, nullptr, nullptr);
                if (ret != SQLITE_OK) {
                    throw std::runtime_error(strprintf("SQLiteDatabase: Failed to create new database: %s\n", sqlite3_errstr(ret)));
                }
            }
        }

        m_db = db;
    }
    SetupSQLStatements();
}

bool SQLiteDatabase::Rewrite(const char* skip)
{
    return false;
}

bool SQLiteDatabase::PeriodicFlush()
{
    return false;
}

bool SQLiteDatabase::Backup(const std::string& dest) const
{
    return false;
}

void SQLiteDatabase::Close()
{
    if (!m_db) return;

    assert(m_refcount == 0);

    // Free all of the prepared statements
    sqlite3_finalize(m_read_stmt);
    sqlite3_finalize(m_insert_stmt);
    sqlite3_finalize(m_overwrite_stmt);
    sqlite3_finalize(m_delete_stmt);
    sqlite3_finalize(m_cursor_stmt);

    int res = sqlite3_close(m_db);
    if (res != SQLITE_OK) {
        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to close database: %s\n", sqlite3_errstr(res)));
    }
    m_db = nullptr;

    UnlockDirectory(m_dir_path, ".walletlock");
}

void SQLiteDatabase::Flush()
{
}

void SQLiteDatabase::ReloadDbEnv()
{
}

void SQLiteDatabase::RemoveRef()
{
    m_refcount--;
}

void SQLiteDatabase::AddRef()
{
    m_refcount++;
}

std::unique_ptr<DatabaseBatch> SQLiteDatabase::MakeBatch(const char* mode, bool flush_on_close)
{
    return nullptr;
}

SQLiteBatch::SQLiteBatch(SQLiteDatabase& database, const char* mode)
    : m_database(database)
{
    m_read_only = (!strchr(mode, '+') && !strchr(mode, 'w'));
    m_database.AddRef();
    m_database.Open(mode);
}

void SQLiteBatch::Flush()
{
}

void SQLiteBatch::Close()
{
    if (m_database.m_db && sqlite3_get_autocommit(m_database.m_db) == 0) {
        TxnAbort();
    }
    m_database.RemoveRef();
}

bool SQLiteBatch::ReadKey(CDataStream&& key, CDataStream& value)
{
    return false;
}

bool SQLiteBatch::WriteKey(CDataStream&& key, CDataStream&& value, bool overwrite)
{
    return false;
}

bool SQLiteBatch::EraseKey(CDataStream&& key)
{
    return false;
}

bool SQLiteBatch::HasKey(CDataStream&& key)
{
    return false;
}

bool SQLiteBatch::StartCursor()
{
    return false;
}

bool SQLiteBatch::ReadAtCursor(CDataStream& key, CDataStream& value, bool& complete)
{
    return false;
}

void SQLiteBatch::CloseCursor()
{
}

bool SQLiteBatch::TxnBegin()
{
    return false;
}

bool SQLiteBatch::TxnCommit()
{
    return false;
}

bool SQLiteBatch::TxnAbort()
{
    return false;
}

bool ExistsSQLiteDatabase(const fs::path& path)
{
    return false;
}

std::unique_ptr<SQLiteDatabase> MakeSQLiteDatabase(const fs::path& path, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error)
{
    return MakeUnique<SQLiteDatabase>(path, path / DATABASE_FILENAME);
}

std::string SQLiteDatabaseVersion()
{
    return std::string(sqlite3_libversion());
}
