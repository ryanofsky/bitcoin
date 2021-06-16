// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_CREATEWALLETDIALOG_H
#define BITCOIN_QT_CREATEWALLETDIALOG_H

#include <QDialog>

<<<<<<< HEAD
||||||| merged common ancestors
class WalletModel;

#ifdef ENABLE_EXTERNAL_SIGNER
=======
class WalletModel;

namespace interfaces {
>>>>>>> Run external signer in bitcoin-wallet process
class ExternalSigner;
<<<<<<< HEAD
class WalletModel;
||||||| merged common ancestors
#endif
=======
} // namespace interfaces
>>>>>>> Run external signer in bitcoin-wallet process

namespace Ui {
    class CreateWalletDialog;
}

/** Dialog for creating wallets
 */
class CreateWalletDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateWalletDialog(QWidget* parent);
    virtual ~CreateWalletDialog();

<<<<<<< HEAD
    void setSigners(const std::vector<ExternalSigner>& signers);
||||||| merged common ancestors
#ifdef ENABLE_EXTERNAL_SIGNER
    void setSigners(std::vector<ExternalSigner>& signers);
#endif
=======
#ifdef ENABLE_EXTERNAL_SIGNER
    void setSigners(std::vector<std::unique_ptr<interfaces::ExternalSigner>>& signers);
#endif
>>>>>>> Run external signer in bitcoin-wallet process

    QString walletName() const;
    bool isEncryptWalletChecked() const;
    bool isDisablePrivateKeysChecked() const;
    bool isMakeBlankWalletChecked() const;
    bool isDescriptorWalletChecked() const;
    bool isExternalSignerChecked() const;

private:
    Ui::CreateWalletDialog *ui;
};

#endif // BITCOIN_QT_CREATEWALLETDIALOG_H
