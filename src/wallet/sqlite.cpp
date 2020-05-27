// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/sqlite.h>

#include <logging.h>
#include <sync.h>
#include <util/memory.h>
#include <util/strencodings.h>
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

SQLiteDatabase::SQLiteDatabase(const fs::path& dir_path, const fs::path& file_path, bool mock) :
    WalletDatabase(), m_mock(mock), m_dir_path(dir_path.string()), m_file_path(file_path.string())
{
    LogPrintf("Using SQLite Version %s\n", SQLiteDatabaseVersion());
    LogPrintf("Using wallet %s\n", m_dir_path);

    LOCK(g_sqlite_mutex);
    assert(m_file_path.empty() || (!m_file_path.empty() && g_file_paths.count(m_file_path) == 0));
    g_file_paths.insert(m_file_path);
}

SQLiteDatabase::~SQLiteDatabase()
{
    Close();
    LOCK(g_sqlite_mutex);
    g_file_paths.erase(m_file_path);
}

void SQLiteDatabase::Open(const char* pszMode)
{
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
}

void SQLiteDatabase::Flush()
{
}

void SQLiteDatabase::ReloadDbEnv()
{
}

void SQLiteDatabase::RemoveRef()
{
}

void SQLiteDatabase::AddRef()
{
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
