// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_DB_H
#define BITCOIN_WALLET_DB_H

#include <fs.h>

#include <string>

/** Given a wallet directory path or legacy file path, return path to main data file in the wallet database. */
fs::path WalletDataFilePath(const fs::path& wallet_path);
<<<<<<< HEAD
void SplitWalletPath(const fs::path& wallet_path, fs::path& env_directory, std::string& database_filename);
||||||| merged common ancestors

/** Get BerkeleyEnvironment and database filename given a wallet path. */
std::shared_ptr<BerkeleyEnvironment> GetWalletEnv(const fs::path& wallet_path, std::string& database_filename);

/** An instance of this class represents one database.
 * For BerkeleyDB this is just a (env, strFile) tuple.
 **/
class BerkeleyDatabase
{
    friend class BerkeleyBatch;
public:
    /** Create dummy DB handle */
    BerkeleyDatabase() : nUpdateCounter(0), nLastSeen(0), nLastFlushed(0), nLastWalletUpdate(0), env(nullptr)
    {
    }

    /** Create DB handle to real database */
    BerkeleyDatabase(std::shared_ptr<BerkeleyEnvironment> env, std::string filename) :
        nUpdateCounter(0), nLastSeen(0), nLastFlushed(0), nLastWalletUpdate(0), env(std::move(env)), strFile(std::move(filename))
    {
        auto inserted = this->env->m_databases.emplace(strFile, std::ref(*this));
        assert(inserted.second);
    }

    ~BerkeleyDatabase() {
        if (env) {
            size_t erased = env->m_databases.erase(strFile);
            assert(erased == 1);
        }
    }

    /** Return object for accessing database at specified path. */
    static std::unique_ptr<BerkeleyDatabase> Create(const fs::path& path)
    {
        std::string filename;
        return MakeUnique<BerkeleyDatabase>(GetWalletEnv(path, filename), std::move(filename));
    }

    /** Return object for accessing dummy database with no read/write capabilities. */
    static std::unique_ptr<BerkeleyDatabase> CreateDummy()
    {
        return MakeUnique<BerkeleyDatabase>();
    }

    /** Return object for accessing temporary in-memory database. */
    static std::unique_ptr<BerkeleyDatabase> CreateMock()
    {
        return MakeUnique<BerkeleyDatabase>(std::make_shared<BerkeleyEnvironment>(), "");
    }

    /** Rewrite the entire database on disk, with the exception of key pszSkip if non-zero
     */
    bool Rewrite(const char* pszSkip=nullptr);

    /** Back up the entire database to a file.
     */
    bool Backup(const std::string& strDest) const;

    /** Make sure all changes are flushed to disk.
     */
    void Flush(bool shutdown);

    void IncrementUpdateCounter();

    void ReloadDbEnv();

    std::atomic<unsigned int> nUpdateCounter;
    unsigned int nLastSeen;
    unsigned int nLastFlushed;
    int64_t nLastWalletUpdate;

    /**
     * Pointer to shared database environment.
     *
     * Normally there is only one BerkeleyDatabase object per
     * BerkeleyEnvivonment, but in the special, backwards compatible case where
     * multiple wallet BDB data files are loaded from the same directory, this
     * will point to a shared instance that gets freed when the last data file
     * is closed.
     */
    std::shared_ptr<BerkeleyEnvironment> env;

    /** Database pointer. This is initialized lazily and reset during flushes, so it can be null. */
    std::unique_ptr<Db> m_db;

private:
    std::string strFile;

    /** Return whether this database handle is a dummy for testing.
     * Only to be used at a low level, application should ideally not care
     * about this.
     */
    bool IsDummy() const { return env == nullptr; }
};

/** RAII class that provides access to a Berkeley database */
class BerkeleyBatch
{
    /** RAII class that automatically cleanses its data on destruction */
    class SafeDbt final
    {
        Dbt m_dbt;

    public:
        // construct Dbt with internally-managed data
        SafeDbt();
        // construct Dbt with provided data
        SafeDbt(void* data, size_t size);
        ~SafeDbt();

        // delegate to Dbt
        const void* get_data() const;
        u_int32_t get_size() const;

        // conversion operator to access the underlying Dbt
        operator Dbt*();
    };

protected:
    Db* pdb;
    std::string strFile;
    DbTxn* activeTxn;
    bool fReadOnly;
    bool fFlushOnClose;
    BerkeleyEnvironment *env;

public:
    explicit BerkeleyBatch(BerkeleyDatabase& database, const char* pszMode = "r+", bool fFlushOnCloseIn=true);
    ~BerkeleyBatch() { Close(); }

    BerkeleyBatch(const BerkeleyBatch&) = delete;
    BerkeleyBatch& operator=(const BerkeleyBatch&) = delete;

    void Flush();
    void Close();

    /* flush the wallet passively (TRY_LOCK)
       ideal to be called periodically */
    static bool PeriodicFlush(BerkeleyDatabase& database);
    /* verifies the database environment */
    static bool VerifyEnvironment(const fs::path& file_path, bilingual_str& errorStr);
    /* verifies the database file */
    static bool VerifyDatabaseFile(const fs::path& file_path, bilingual_str& errorStr);

    template <typename K, typename T>
    bool Read(const K& key, T& value)
    {
        if (!pdb)
            return false;

        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        SafeDbt datKey(ssKey.data(), ssKey.size());

        // Read
        SafeDbt datValue;
        int ret = pdb->get(activeTxn, datKey, datValue, 0);
        bool success = false;
        if (datValue.get_data() != nullptr) {
            // Unserialize value
            try {
                CDataStream ssValue((char*)datValue.get_data(), (char*)datValue.get_data() + datValue.get_size(), SER_DISK, CLIENT_VERSION);
                ssValue >> value;
                success = true;
            } catch (const std::exception&) {
                // In this case success remains 'false'
            }
        }
        return ret == 0 && success;
    }

    template <typename K, typename T>
    bool Write(const K& key, const T& value, bool fOverwrite = true)
    {
        if (!pdb)
            return true;
        if (fReadOnly)
            assert(!"Write called on database in read-only mode");

        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        SafeDbt datKey(ssKey.data(), ssKey.size());

        // Value
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.reserve(10000);
        ssValue << value;
        SafeDbt datValue(ssValue.data(), ssValue.size());

        // Write
        int ret = pdb->put(activeTxn, datKey, datValue, (fOverwrite ? 0 : DB_NOOVERWRITE));
        return (ret == 0);
    }

    template <typename K>
    bool Erase(const K& key)
    {
        if (!pdb)
            return false;
        if (fReadOnly)
            assert(!"Erase called on database in read-only mode");

        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        SafeDbt datKey(ssKey.data(), ssKey.size());

        // Erase
        int ret = pdb->del(activeTxn, datKey, 0);
        return (ret == 0 || ret == DB_NOTFOUND);
    }

    template <typename K>
    bool Exists(const K& key)
    {
        if (!pdb)
            return false;

        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        SafeDbt datKey(ssKey.data(), ssKey.size());

        // Exists
        int ret = pdb->exists(activeTxn, datKey, 0);
        return (ret == 0);
    }

    Dbc* GetCursor()
    {
        if (!pdb)
            return nullptr;
        Dbc* pcursor = nullptr;
        int ret = pdb->cursor(nullptr, &pcursor, 0);
        if (ret != 0)
            return nullptr;
        return pcursor;
    }

    int ReadAtCursor(Dbc* pcursor, CDataStream& ssKey, CDataStream& ssValue)
    {
        // Read at cursor
        SafeDbt datKey;
        SafeDbt datValue;
        int ret = pcursor->get(datKey, datValue, DB_NEXT);
        if (ret != 0)
            return ret;
        else if (datKey.get_data() == nullptr || datValue.get_data() == nullptr)
            return 99999;

        // Convert to streams
        ssKey.SetType(SER_DISK);
        ssKey.clear();
        ssKey.write((char*)datKey.get_data(), datKey.get_size());
        ssValue.SetType(SER_DISK);
        ssValue.clear();
        ssValue.write((char*)datValue.get_data(), datValue.get_size());
        return 0;
    }

    bool TxnBegin()
    {
        if (!pdb || activeTxn)
            return false;
        DbTxn* ptxn = env->TxnBegin();
        if (!ptxn)
            return false;
        activeTxn = ptxn;
        return true;
    }

    bool TxnCommit()
    {
        if (!pdb || !activeTxn)
            return false;
        int ret = activeTxn->commit(0);
        activeTxn = nullptr;
        return (ret == 0);
    }

    bool TxnAbort()
    {
        if (!pdb || !activeTxn)
            return false;
        int ret = activeTxn->abort();
        activeTxn = nullptr;
        return (ret == 0);
    }

    bool static Rewrite(BerkeleyDatabase& database, const char* pszSkip = nullptr);
};

std::string BerkeleyDatabaseVersion();
=======

/** Get BerkeleyEnvironment and database filename given a wallet path. */
std::shared_ptr<BerkeleyEnvironment> GetWalletEnv(const fs::path& wallet_path, std::string& database_filename);

/** An instance of this class represents one database.
 * For BerkeleyDB this is just a (env, strFile) tuple.
 **/
class BerkeleyDatabase
{
    friend class BerkeleyBatch;
public:
    /** Create dummy DB handle */
    BerkeleyDatabase() : nUpdateCounter(0), nLastSeen(0), nLastFlushed(0), nLastWalletUpdate(0), env(nullptr)
    {
    }

    /** Create DB handle to real database */
    BerkeleyDatabase(std::shared_ptr<BerkeleyEnvironment> env, std::string filename) :
        nUpdateCounter(0), nLastSeen(0), nLastFlushed(0), nLastWalletUpdate(0), env(std::move(env)), strFile(std::move(filename))
    {
        auto inserted = this->env->m_databases.emplace(strFile, std::ref(*this));
        assert(inserted.second);
    }

    ~BerkeleyDatabase() {
        if (env) {
            size_t erased = env->m_databases.erase(strFile);
            assert(erased == 1);
        }
    }

    /** Return object for accessing database at specified path. */
    static std::unique_ptr<BerkeleyDatabase> Create(const fs::path& path)
    {
        std::string filename;
        return MakeUnique<BerkeleyDatabase>(GetWalletEnv(path, filename), std::move(filename));
    }

    /** Return object for accessing dummy database with no read/write capabilities. */
    static std::unique_ptr<BerkeleyDatabase> CreateDummy()
    {
        return MakeUnique<BerkeleyDatabase>();
    }

    /** Return object for accessing temporary in-memory database. */
    static std::unique_ptr<BerkeleyDatabase> CreateMock()
    {
        return MakeUnique<BerkeleyDatabase>(std::make_shared<BerkeleyEnvironment>(), "");
    }

    /** Rewrite the entire database on disk, with the exception of key pszSkip if non-zero
     */
    bool Rewrite(const char* pszSkip=nullptr);

    /** Back up the entire database to a file.
     */
    bool Backup(const std::string& strDest) const;

    /** Make sure all changes are flushed to disk.
     */
    void Flush(bool shutdown);

    void IncrementUpdateCounter();

    void ReloadDbEnv();

    std::atomic<unsigned int> nUpdateCounter;
    unsigned int nLastSeen;
    unsigned int nLastFlushed;
    int64_t nLastWalletUpdate;

    /**
     * Pointer to shared database environment.
     *
     * Normally there is only one BerkeleyDatabase object per
     * BerkeleyEnvivonment, but in the special, backwards compatible case where
     * multiple wallet BDB data files are loaded from the same directory, this
     * will point to a shared instance that gets freed when the last data file
     * is closed.
     */
    std::shared_ptr<BerkeleyEnvironment> env;

    /** Database pointer. This is initialized lazily and reset during flushes, so it can be null. */
    std::unique_ptr<Db> m_db;

private:
    std::string strFile;

    /** Return whether this database handle is a dummy for testing.
     * Only to be used at a low level, application should ideally not care
     * about this.
     */
    bool IsDummy() const { return env == nullptr; }
};

/** RAII class that provides access to a Berkeley database */
class BerkeleyBatch
{
    /** RAII class that automatically cleanses its data on destruction */
    class SafeDbt final
    {
        Dbt m_dbt;

    public:
        // construct Dbt with internally-managed data
        SafeDbt();
        // construct Dbt with provided data
        SafeDbt(void* data, size_t size);
        ~SafeDbt();

        // delegate to Dbt
        const void* get_data() const;
        u_int32_t get_size() const;

        // conversion operator to access the underlying Dbt
        operator Dbt*();
    };

protected:
    Db* pdb;
    std::string strFile;
    DbTxn* activeTxn;
    bool fReadOnly;
    bool fFlushOnClose;
    BerkeleyEnvironment *env;

public:
    explicit BerkeleyBatch(BerkeleyDatabase& database, const char* pszMode = "r+", bool fFlushOnCloseIn=true);
    ~BerkeleyBatch() { Close(); }

    BerkeleyBatch(const BerkeleyBatch&) = delete;
    BerkeleyBatch& operator=(const BerkeleyBatch&) = delete;

    void Flush();
    void Close();

    /* flush the wallet passively (TRY_LOCK)
       ideal to be called periodically */
    static bool PeriodicFlush(BerkeleyDatabase& database);
    /* verifies the database environment */
    static bool VerifyEnvironment(const fs::path& file_path, bilingual_str& errorStr);
    /* verifies the database file */
    static bool VerifyDatabaseFile(const fs::path& file_path, bilingual_str& errorStr);

    template <typename K, typename T>
    bool Read(const K& key, T& value)
    {
        if (!pdb)
            return false;

        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        SafeDbt datKey(ssKey.data(), ssKey.size());

        // Read
        SafeDbt datValue;
        int ret = pdb->get(activeTxn, datKey, datValue, 0);
        bool success = false;
        if (datValue.get_data() != nullptr) {
            // Unserialize value
            try {
                CDataStream ssValue((char*)datValue.get_data(), (char*)datValue.get_data() + datValue.get_size(), SER_DISK, CLIENT_VERSION);
                ssValue >> value;
                success = true;
            } catch (const std::exception&) {
                // In this case success remains 'false'
            }
        }
        return ret == 0 && success;
    }

    template <typename K, typename T>
    bool Write(const K& key, const T& value, bool fOverwrite = true)
    {
        if (!pdb)
            return true;
        if (fReadOnly)
            assert(!"Write called on database in read-only mode");

        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        SafeDbt datKey(ssKey.data(), ssKey.size());

        // Value
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.reserve(10000);
        ssValue << value;
        SafeDbt datValue(ssValue.data(), ssValue.size());

        // Write
        int ret = pdb->put(activeTxn, datKey, datValue, (fOverwrite ? 0 : DB_NOOVERWRITE));
        return (ret == 0);
    }

    template <typename K>
    bool Erase(const K& key)
    {
        if (!pdb)
            return false;
        if (fReadOnly)
            assert(!"Erase called on database in read-only mode");

        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        SafeDbt datKey(ssKey.data(), ssKey.size());

        // Erase
        int ret = pdb->del(activeTxn, datKey, 0);
        return (ret == 0 || ret == DB_NOTFOUND);
    }

    template <typename K>
    bool Exists(const K& key)
    {
        if (!pdb)
            return false;

        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        SafeDbt datKey(ssKey.data(), ssKey.size());

        // Exists
        int ret = pdb->exists(activeTxn, datKey, 0);
        return (ret == 0);
    }

    Dbc* GetCursor()
    {
        if (!pdb)
            return nullptr;
        Dbc* pcursor = nullptr;
        int ret = pdb->cursor(activeTxn, &pcursor, 0);
        if (ret != 0)
            return nullptr;
        return pcursor;
    }

    int ReadAtCursor(Dbc* pcursor, CDataStream& ssKey, CDataStream& ssValue)
    {
        // Read at cursor
        SafeDbt datKey;
        SafeDbt datValue;
        int ret = pcursor->get(datKey, datValue, DB_NEXT);
        if (ret != 0)
            return ret;
        else if (datKey.get_data() == nullptr || datValue.get_data() == nullptr)
            return 99999;

        // Convert to streams
        ssKey.SetType(SER_DISK);
        ssKey.clear();
        ssKey.write((char*)datKey.get_data(), datKey.get_size());
        ssValue.SetType(SER_DISK);
        ssValue.clear();
        ssValue.write((char*)datValue.get_data(), datValue.get_size());
        return 0;
    }

    bool ErasePrefix(const char* data, size_t size)
    {
        TxnBegin();
        Dbc* pcursor = GetCursor();
        Dbt prefix((void*)data, size), prefix_value;
        int ret = pcursor->get(&prefix, &prefix_value, DB_SET_RANGE);
        for (int flag = DB_CURRENT; ret == 0; flag = DB_NEXT) {
            SafeDbt key, value;
            if ((ret = pcursor->get(key, value, flag)) != 0 || key.get_size() < size || memcmp(key.get_data(), data, size) != 0) break;
            pcursor->del(0);
        }
        pcursor->close();
        TxnCommit();
        return ret == 0 || ret == DB_NOTFOUND;
    }

    bool TxnBegin()
    {
        if (!pdb || activeTxn)
            return false;
        DbTxn* ptxn = env->TxnBegin();
        if (!ptxn)
            return false;
        activeTxn = ptxn;
        return true;
    }

    bool TxnCommit()
    {
        if (!pdb || !activeTxn)
            return false;
        int ret = activeTxn->commit(0);
        activeTxn = nullptr;
        return (ret == 0);
    }

    bool TxnAbort()
    {
        if (!pdb || !activeTxn)
            return false;
        int ret = activeTxn->abort();
        activeTxn = nullptr;
        return (ret == 0);
    }

    bool static Rewrite(BerkeleyDatabase& database, const char* pszSkip = nullptr);
};

std::string BerkeleyDatabaseVersion();
>>>>>>> refactor: Remove CAddressBookData::destdata

#endif // BITCOIN_WALLET_DB_H
