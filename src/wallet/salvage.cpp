// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <streams.h>
#include <util/fs.h>
#include <util/translation.h>
#include <wallet/bdb.h>
#include <wallet/salvage.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h>

#include <db_cxx.h>

namespace wallet {
/* End of headers, beginning of key/value data */
static const char *HEADER_END = "HEADER=END";
/* End of key/value data */
static const char *DATA_END = "DATA=END";
typedef std::pair<std::vector<unsigned char>, std::vector<unsigned char> > KeyValPair;

class DummyCursor : public DatabaseCursor
{
    Status Next(DataStream& key, DataStream& value) override { return Status::FAIL; }
};

/** RAII class that provides access to a DummyDatabase. Never fails. */
class DummyBatch : public DatabaseBatch
{
private:
    bool ReadKey(DataStream&& key, DataStream& value) override { return true; }
    bool WriteKey(DataStream&& key, DataStream&& value, bool overwrite=true) override { return true; }
    bool EraseKey(DataStream&& key) override { return true; }
    bool HasKey(DataStream&& key) override { return true; }
    bool ErasePrefix(Span<const std::byte> prefix) override { return true; }

public:
    void Flush() override {}
    void Close() override {}

    std::unique_ptr<DatabaseCursor> GetNewCursor() override { return std::make_unique<DummyCursor>(); }
    std::unique_ptr<DatabaseCursor> GetNewPrefixCursor(Span<const std::byte> prefix) override { return GetNewCursor(); }
    bool TxnBegin() override { return true; }
    bool TxnCommit() override { return true; }
    bool TxnAbort() override { return true; }
    bool HasActiveTxn() override { return false; }
};

/** A dummy WalletDatabase that does nothing and never fails. Only used by salvage.
 **/
class DummyDatabase : public WalletDatabase
{
public:
    void Open() override {};
    void AddRef() override {}
    void RemoveRef() override {}
    bool Rewrite(const char* pszSkip=nullptr) override { return true; }
    bool Backup(const std::string& strDest) const override { return true; }
    void Close() override {}
    void Flush() override {}
    bool PeriodicFlush() override { return true; }
    void IncrementUpdateCounter() override { ++nUpdateCounter; }
    void ReloadDbEnv() override {}
    std::string Filename() override { return "dummy"; }
    std::string Format() override { return "dummy"; }
    std::unique_ptr<DatabaseBatch> MakeBatch(bool flush_on_close = true) override { return std::make_unique<DummyBatch>(); }
};

util::Result<void> RecoverDatabaseFile(const ArgsManager& args, const fs::path& file_path)
{
    util::Result<void> result;
    DatabaseOptions options;
    ReadDatabaseArgs(args, options);
    options.require_existing = true;
    options.verify = false;
    options.require_format = DatabaseFormat::BERKELEY;
    auto database{MakeDatabase(file_path, options) >> result};
    if (!database) {
        result.Update(util::Error{});
        return result;
    }

    BerkeleyDatabase& berkeley_database = static_cast<BerkeleyDatabase&>(*database);
    std::string filename = berkeley_database.Filename();
    std::shared_ptr<BerkeleyEnvironment> env = berkeley_database.env;

    if (!(env->Open() >> result)) {
        result.Update(util::Error{});
        return result;
    }

    // Recovery procedure:
    // move wallet file to walletfilename.timestamp.bak
    // Call Salvage with fAggressive=true to
    // get as much data as possible.
    // Rewrite salvaged data to fresh wallet file
    // Rescan so any missing transactions will be
    // found.
    int64_t now = GetTime();
    std::string newFilename = strprintf("%s.%d.bak", filename, now);

    int ret = env->dbenv->dbrename(nullptr, filename.c_str(), nullptr,
                                   newFilename.c_str(), DB_AUTO_COMMIT);
    if (ret != 0)
    {
<<<<<<< HEAD
        error = Untranslated(strprintf("Failed to rename %s to %s", filename, newFilename));
        return false;
||||||| parent of 193c85c84179 (refactor: Use util::Result class in wallet/salvage)
        error = strprintf(Untranslated("Failed to rename %s to %s"), filename, newFilename);
        return false;
=======
        result.Update(util::Error{strprintf(Untranslated("Failed to rename %s to %s"), filename, newFilename)});
        return result;
>>>>>>> 193c85c84179 (refactor: Use util::Result class in wallet/salvage)
    }

    /**
     * Salvage data from a file. The DB_AGGRESSIVE flag is being used (see berkeley DB->verify() method documentation).
     * key/value pairs are appended to salvagedData which are then written out to a new wallet file.
     * NOTE: reads the entire database into memory, so cannot be used
     * for huge databases.
     */
    std::vector<KeyValPair> salvagedData;

    std::stringstream strDump;

    Db db(env->dbenv.get(), 0);
<<<<<<< HEAD
    result = db.verify(newFilename.c_str(), nullptr, &strDump, DB_SALVAGE | DB_AGGRESSIVE);
    if (result == DB_VERIFY_BAD) {
        warnings.emplace_back(Untranslated("Salvage: Database salvage found errors, all data may not be recoverable."));
||||||| parent of 193c85c84179 (refactor: Use util::Result class in wallet/salvage)
    result = db.verify(newFilename.c_str(), nullptr, &strDump, DB_SALVAGE | DB_AGGRESSIVE);
    if (result == DB_VERIFY_BAD) {
        warnings.push_back(Untranslated("Salvage: Database salvage found errors, all data may not be recoverable."));
=======
    ret = db.verify(newFilename.c_str(), nullptr, &strDump, DB_SALVAGE | DB_AGGRESSIVE);
    if (ret == DB_VERIFY_BAD) {
        result.AddWarning(Untranslated("Salvage: Database salvage found errors, all data may not be recoverable."));
>>>>>>> 193c85c84179 (refactor: Use util::Result class in wallet/salvage)
    }
<<<<<<< HEAD
    if (result != 0 && result != DB_VERIFY_BAD) {
        error = Untranslated(strprintf("Salvage: Database salvage failed with result %d.", result));
        return false;
||||||| parent of 193c85c84179 (refactor: Use util::Result class in wallet/salvage)
    if (result != 0 && result != DB_VERIFY_BAD) {
        error = strprintf(Untranslated("Salvage: Database salvage failed with result %d."), result);
        return false;
=======
    if (ret != 0 && ret != DB_VERIFY_BAD) {
        result.Update(util::Error{strprintf(Untranslated("Salvage: Database salvage failed with result %d."), ret)});
        return result;
>>>>>>> 193c85c84179 (refactor: Use util::Result class in wallet/salvage)
    }

    // Format of bdb dump is ascii lines:
    // header lines...
    // HEADER=END
    //  hexadecimal key
    //  hexadecimal value
    //  ... repeated
    // DATA=END

    std::string strLine;
    while (!strDump.eof() && strLine != HEADER_END)
        getline(strDump, strLine); // Skip past header

    std::string keyHex, valueHex;
    while (!strDump.eof() && keyHex != DATA_END) {
        getline(strDump, keyHex);
        if (keyHex != DATA_END) {
            if (strDump.eof())
                break;
            getline(strDump, valueHex);
            if (valueHex == DATA_END) {
<<<<<<< HEAD
                warnings.emplace_back(Untranslated("Salvage: WARNING: Number of keys in data does not match number of values."));
||||||| parent of 193c85c84179 (refactor: Use util::Result class in wallet/salvage)
                warnings.push_back(Untranslated("Salvage: WARNING: Number of keys in data does not match number of values."));
=======
                result.AddWarning(Untranslated("Salvage: WARNING: Number of keys in data does not match number of values."));
>>>>>>> 193c85c84179 (refactor: Use util::Result class in wallet/salvage)
                break;
            }
            salvagedData.emplace_back(ParseHex(keyHex), ParseHex(valueHex));
        }
    }

    if (keyHex != DATA_END) {
<<<<<<< HEAD
        warnings.emplace_back(Untranslated("Salvage: WARNING: Unexpected end of file while reading salvage output."));
        fSuccess = false;
    } else {
        fSuccess = (result == 0);
||||||| parent of 193c85c84179 (refactor: Use util::Result class in wallet/salvage)
        warnings.push_back(Untranslated("Salvage: WARNING: Unexpected end of file while reading salvage output."));
        fSuccess = false;
    } else {
        fSuccess = (result == 0);
=======
        result.Update({util::Error{}, util::Warning{Untranslated("Salvage: WARNING: Unexpected end of file while reading salvage output.")}});
    } else if (ret != 0) {
        result.Update(util::Error{});
>>>>>>> 193c85c84179 (refactor: Use util::Result class in wallet/salvage)
    }

    if (salvagedData.empty())
    {
<<<<<<< HEAD
        error = Untranslated(strprintf("Salvage(aggressive) found no records in %s.", newFilename));
        return false;
||||||| parent of 193c85c84179 (refactor: Use util::Result class in wallet/salvage)
        error = strprintf(Untranslated("Salvage(aggressive) found no records in %s."), newFilename);
        return false;
=======
        result.Update(util::Error{strprintf(Untranslated("Salvage(aggressive) found no records in %s."), newFilename)});
        return result;
>>>>>>> 193c85c84179 (refactor: Use util::Result class in wallet/salvage)
    }

    std::unique_ptr<Db> pdbCopy = std::make_unique<Db>(env->dbenv.get(), 0);
    ret = pdbCopy->open(nullptr,                // Txn pointer
                            filename.c_str(),   // Filename
                            "main",             // Logical db name
                            DB_BTREE,           // Database type
                            DB_CREATE,          // Flags
                            0);
    if (ret > 0) {
<<<<<<< HEAD
        error = Untranslated(strprintf("Cannot create database file %s", filename));
||||||| parent of 193c85c84179 (refactor: Use util::Result class in wallet/salvage)
        error = strprintf(Untranslated("Cannot create database file %s"), filename);
=======
>>>>>>> 193c85c84179 (refactor: Use util::Result class in wallet/salvage)
        pdbCopy->close(0);
        result.Update(util::Error{strprintf(Untranslated("Cannot create database file %s"), filename)});
        return result;
    }

    DbTxn* ptxn = env->TxnBegin(DB_TXN_WRITE_NOSYNC);
    CWallet dummyWallet(nullptr, "", std::make_unique<DummyDatabase>());
    for (KeyValPair& row : salvagedData)
    {
        /* Filter for only private key type KV pairs to be added to the salvaged wallet */
        DataStream ssKey{row.first};
        DataStream ssValue(row.second);
        std::string strType, strErr;

        // We only care about KEY, MASTER_KEY, CRYPTED_KEY, and HDCHAIN types
        ssKey >> strType;
        bool fReadOK = false;
        if (strType == DBKeys::KEY) {
            fReadOK = LoadKey(&dummyWallet, ssKey, ssValue, strErr);
        } else if (strType == DBKeys::CRYPTED_KEY) {
            fReadOK = LoadCryptedKey(&dummyWallet, ssKey, ssValue, strErr);
        } else if (strType == DBKeys::MASTER_KEY) {
            fReadOK = LoadEncryptionKey(&dummyWallet, ssKey, ssValue, strErr);
        } else if (strType == DBKeys::HDCHAIN) {
            fReadOK = LoadHDChain(&dummyWallet, ssValue, strErr);
        } else {
            continue;
        }

        if (!fReadOK)
        {
<<<<<<< HEAD
            warnings.push_back(Untranslated(strprintf("WARNING: WalletBatch::Recover skipping %s: %s", strType, strErr)));
||||||| parent of 193c85c84179 (refactor: Use util::Result class in wallet/salvage)
            warnings.push_back(strprintf(Untranslated("WARNING: WalletBatch::Recover skipping %s: %s"), strType, strErr));
=======
            result.AddWarning(strprintf(Untranslated("WARNING: WalletBatch::Recover skipping %s: %s"), strType, strErr));
>>>>>>> 193c85c84179 (refactor: Use util::Result class in wallet/salvage)
            continue;
        }
        Dbt datKey(row.first.data(), row.first.size());
        Dbt datValue(row.second.data(), row.second.size());
        int ret2 = pdbCopy->put(ptxn, &datKey, &datValue, DB_NOOVERWRITE);
        if (ret2 > 0) {
            result.Update(util::Error{});
        }
    }
    ptxn->commit(0);
    pdbCopy->close(0);

    return result;
}
} // namespace wallet
