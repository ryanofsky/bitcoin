#ifndef ENTERPRISE_TRANSACTIONS_H
#define ENTERPRISE_TRANSACTIONS_H

#include <odb/core.hxx>
#include <string>

#pragma db object

class etransactions {
public:
    etransactions(const std::string &txid,
                unsigned int satoshis)
            : txid_(txid), satoshis_(satoshis) {
    }

    const std::string &
    txid() const {
        return txid_;
    }

    unsigned int
    satoshis() const {
        return satoshis_;
    }

private:
    friend class odb::access;

    etransactions() {}

#pragma db id auto
    unsigned int id_;

    std::string txid_;
    unsigned int satoshis_;
};

#endif //ENTERPRISE_TRANSACTIONS_H
