#include "Wallet.h"
#include "PendingTransaction.h"
#include "UnsignedTransaction.h"
#include "TransactionHistory.h"
#include "AddressBook.h"
#include "Subaddress.h"
#include "model/TransactionHistoryModel.h"
#include "model/TransactionHistorySortFilterModel.h"
#include "model/AddressBookModel.h"
#include "model/SubaddressModel.h"
#include "wallet/api/wallet2_api.h"

#include <QFile>
#include <QDir>
#include <QDebug>
#include <QUrl>
#include <QTimer>
//#if QT_MAJOR_VERSION >= 5
  #include <QtConcurrent/QtConcurrent>
//#else
//  #include <QtCore>
//#endif
#include <QList>
#include <QVector>
#include <QMutex>
#include <QMutexLocker>

namespace {
    static const int DAEMON_BLOCKCHAIN_HEIGHT_CACHE_TTL_SECONDS = 5;
    static const int DAEMON_BLOCKCHAIN_TARGET_HEIGHT_CACHE_TTL_SECONDS = 30;
    static const int WALLET_CONNECTION_STATUS_CACHE_TTL_SECONDS = 5;
}

class WalletListenerImpl : public  Monero::WalletListener
{
public:
    WalletListenerImpl(Wallet * w)
        : m_wallet(w)
    {

    }

    virtual void moneySpent(const std::string &txId, uint64_t amount)
    {
        qDebug() << __FUNCTION__;
        emit m_wallet->moneySpent(QString::fromStdString(txId), amount);
    }


    virtual void moneyReceived(const std::string &txId, uint64_t amount)
    {
        qDebug() << __FUNCTION__;
        emit m_wallet->moneyReceived(QString::fromStdString(txId), amount);
    }

    virtual void unconfirmedMoneyReceived(const std::string &txId, uint64_t amount)
    {
        qDebug() << __FUNCTION__;
        emit m_wallet->unconfirmedMoneyReceived(QString::fromStdString(txId), amount);
    }

    virtual void newBlock(uint64_t height)
    {
        // qDebug() << __FUNCTION__;
        emit m_wallet->newBlock(height, m_wallet->daemonBlockChainTargetHeight());
    }

    virtual void updated()
    {
        emit m_wallet->updated();
    }

    // called when wallet refreshed by background thread or explicitly
    virtual void refreshed()
    {
        qDebug() << __FUNCTION__;
        emit m_wallet->refreshed();
    }

private:
    Wallet * m_wallet;
};

Wallet::Wallet(QObject * parent)
    : Wallet(nullptr, parent)
{
}

QString Wallet::getSeed() const
{
    return QString::fromStdString(m_walletImpl->seed());
}

QString Wallet::getSeedLanguage() const
{
    return QString::fromStdString(m_walletImpl->getSeedLanguage());
}

void Wallet::setSeedLanguage(const QString &lang)
{
    m_walletImpl->setSeedLanguage(lang.toStdString());
}

Wallet::Status Wallet::status() const
{
    return static_cast<Status>(m_walletImpl->status());
}

NetworkType::Type Wallet::nettype() const
{
    return static_cast<NetworkType::Type>(m_walletImpl->nettype());
}


void Wallet::updateConnectionStatusAsync()
{
    QFuture<Monero::Wallet::ConnectionStatus> future = QtConcurrent::run(m_walletImpl, &Monero::Wallet::connected);
    QFutureWatcher<Monero::Wallet::ConnectionStatus> *connectionWatcher = new QFutureWatcher<Monero::Wallet::ConnectionStatus>();

    connect(connectionWatcher, &QFutureWatcher<Monero::Wallet::ConnectionStatus>::finished, [=]() {
        QFuture<Monero::Wallet::ConnectionStatus> future = connectionWatcher->future();
        connectionWatcher->deleteLater();
        ConnectionStatus newStatus = static_cast<ConnectionStatus>(future.result());
        if (newStatus != m_connectionStatus || !m_initialized) {
            m_initialized = true;
            m_connectionStatus = newStatus;
            qDebug() << "NEW STATUS " << newStatus;
            emit connectionStatusChanged(newStatus);
        }
        // Release lock
        m_connectionStatusRunning = false;
    });
    connectionWatcher->setFuture(future);
}

Wallet::ConnectionStatus Wallet::connected(bool forceCheck)
{
    // cache connection status
    if (forceCheck || !m_initialized || (m_connectionStatusTime.elapsed() / 1000 > m_connectionStatusTtl && !m_connectionStatusRunning) || m_connectionStatusTime.elapsed() > 30000) {
        qDebug() << "Checking connection status";
        m_connectionStatusRunning = true;
        m_connectionStatusTime.restart();
        updateConnectionStatusAsync();
    }

    return m_connectionStatus;
}

bool Wallet::synchronized() const
{
    return m_walletImpl->synchronized();
}

QString Wallet::errorString() const
{
    return QString::fromStdString(m_walletImpl->errorString());
}

bool Wallet::setPassword(const QString &password)
{
    return m_walletImpl->setPassword(password.toStdString());
}

QString Wallet::address(quint32 accountIndex, quint32 addressIndex) const
{
    return QString::fromStdString(m_walletImpl->address(accountIndex, addressIndex));
}

QString Wallet::path() const
{
    return QString::fromStdString(m_walletImpl->path());
}

bool Wallet::store(const QString &path)
{
    return m_walletImpl->store(path.toStdString());
}

bool Wallet::init(const QString &daemonAddress, quint64 upperTransactionLimit, bool isRecovering, quint64 restoreHeight)
{
    qDebug() << "init non async";
    if (isRecovering){
        qDebug() << "RESTORING";
        m_walletImpl->setRecoveringFromSeed(true);
        m_walletImpl->setRefreshFromBlockHeight(restoreHeight);
    }
    m_walletImpl->init(daemonAddress.toStdString(), upperTransactionLimit, m_daemonUsername.toStdString(), m_daemonPassword.toStdString());
    return true;
}

void Wallet::setDaemonLogin(const QString &daemonUsername, const QString &daemonPassword)
{
    // store daemon login
    m_daemonUsername = daemonUsername;
    m_daemonPassword = daemonPassword;
}

void Wallet::initAsync(const QString &daemonAddress, quint64 upperTransactionLimit, bool isRecovering, quint64 restoreHeight)
{
    qDebug() << "initAsync: " + daemonAddress;
    // Change status to disconnected if connected
    if(m_connectionStatus != Wallet::ConnectionStatus_Disconnected) {
        m_connectionStatus = Wallet::ConnectionStatus_Disconnected;
        emit connectionStatusChanged(m_connectionStatus);
    }

    QFuture<bool> future = QtConcurrent::run(this, &Wallet::init,
                                  daemonAddress, upperTransactionLimit, isRecovering, restoreHeight);
    QFutureWatcher<bool> * watcher = new QFutureWatcher<bool>();

    connect(watcher, &QFutureWatcher<bool>::finished,
            this, [this, watcher, daemonAddress, upperTransactionLimit, isRecovering, restoreHeight]() {
        QFuture<bool> future = watcher->future();
        watcher->deleteLater();
        if(future.result()){
            emit walletCreationHeightChanged();
            qDebug() << "init async finished - starting refresh";
            connected(true);
            m_walletImpl->startRefresh();

        }
    });
    watcher->setFuture(future);
}

//! create a view only wallet
bool Wallet::createViewOnly(const QString &path, const QString &password) const
{
    // Create path
    QDir d = QFileInfo(path).absoluteDir();
    d.mkpath(d.absolutePath());
    return m_walletImpl->createWatchOnly(path.toStdString(),password.toStdString(),m_walletImpl->getSeedLanguage());
}

bool Wallet::connectToDaemon()
{
    return m_walletImpl->connectToDaemon();
}

void Wallet::setTrustedDaemon(bool arg)
{
    m_walletImpl->setTrustedDaemon(arg);
}

bool Wallet::viewOnly() const
{
    return m_walletImpl->watchOnly();
}

quint64 Wallet::balance(quint32 accountIndex) const
{
    return m_walletImpl->balance(accountIndex);
}

quint64 Wallet::balanceAll() const
{
    return m_walletImpl->balanceAll();
}

quint64 Wallet::unlockedBalance(quint32 accountIndex) const
{
    return m_walletImpl->unlockedBalance(accountIndex);
}

quint64 Wallet::unlockedBalanceAll() const
{
    return m_walletImpl->unlockedBalanceAll();
}

quint32 Wallet::currentSubaddressAccount() const
{
    return m_currentSubaddressAccount;
}
void Wallet::switchSubaddressAccount(quint32 accountIndex)
{
    if (accountIndex < numSubaddressAccounts())
    {
        m_currentSubaddressAccount = accountIndex;
        m_subaddress->refresh(m_currentSubaddressAccount);
        m_history->refresh(m_currentSubaddressAccount);
    }
}
void Wallet::addSubaddressAccount(const QString& label)
{
    m_walletImpl->addSubaddressAccount(label.toStdString());
    switchSubaddressAccount(numSubaddressAccounts() - 1);
}
quint32 Wallet::numSubaddressAccounts() const
{
    return m_walletImpl->numSubaddressAccounts();
}
quint32 Wallet::numSubaddresses(quint32 accountIndex) const
{
    return m_walletImpl->numSubaddresses(accountIndex);
}
void Wallet::addSubaddress(const QString& label)
{
    m_walletImpl->addSubaddress(currentSubaddressAccount(), label.toStdString());
}
QString Wallet::getSubaddressLabel(quint32 accountIndex, quint32 addressIndex) const
{
    return QString::fromStdString(m_walletImpl->getSubaddressLabel(accountIndex, addressIndex));
}
void Wallet::setSubaddressLabel(quint32 accountIndex, quint32 addressIndex, const QString &label)
{
    m_walletImpl->setSubaddressLabel(accountIndex, addressIndex, label.toStdString());
}

quint64 Wallet::blockChainHeight() const
{
    return m_walletImpl->blockChainHeight();
}

quint64 Wallet::daemonBlockChainHeight() const
{
    // cache daemon blockchain height for some time (60 seconds by default)

    if (m_daemonBlockChainHeight == 0
            || m_daemonBlockChainHeightTime.elapsed() / 1000 > m_daemonBlockChainHeightTtl) {
        m_daemonBlockChainHeight = m_walletImpl->daemonBlockChainHeight();
        m_daemonBlockChainHeightTime.restart();
    }
    return m_daemonBlockChainHeight;
}

quint64 Wallet::daemonBlockChainTargetHeight() const
{
    if (m_daemonBlockChainTargetHeight <= 1
            || m_daemonBlockChainTargetHeightTime.elapsed() / 1000 > m_daemonBlockChainTargetHeightTtl) {
        m_daemonBlockChainTargetHeight = m_walletImpl->daemonBlockChainTargetHeight();

        // Target height is set to 0 if daemon is synced.
        // Use current height from daemon when target height < current height
        if (m_daemonBlockChainTargetHeight < m_daemonBlockChainHeight){
            m_daemonBlockChainTargetHeight = m_daemonBlockChainHeight;
        }
        m_daemonBlockChainTargetHeightTime.restart();
    }

    return m_daemonBlockChainTargetHeight;
}

bool Wallet::refresh()
{
    bool result = m_walletImpl->refresh();
    m_history->refresh(currentSubaddressAccount());
    m_subaddress->refresh(currentSubaddressAccount());
    if (result)
        emit updated();
    return result;
}

void Wallet::refreshAsync()
{
    qDebug() << "refresh async";
    m_walletImpl->refreshAsync();
}

void Wallet::setAutoRefreshInterval(int seconds)
{
    m_walletImpl->setAutoRefreshInterval(seconds);
}

int Wallet::autoRefreshInterval() const
{
    return m_walletImpl->autoRefreshInterval();
}

void Wallet::startRefresh() const
{
    m_walletImpl->startRefresh();
}

void Wallet::pauseRefresh() const
{
    m_walletImpl->pauseRefresh();
}

PendingTransaction *Wallet::createTransaction(const QString &dst_addr, const QString &payment_id,
                                              quint64 amount, quint32 mixin_count,
                                              PendingTransaction::Priority priority)
{
    std::set<uint32_t> subaddr_indices;
    Monero::PendingTransaction * ptImpl = m_walletImpl->createTransaction(
                dst_addr.toStdString(), payment_id.toStdString(), amount, mixin_count,
                static_cast<Monero::PendingTransaction::Priority>(priority), currentSubaddressAccount(), subaddr_indices);
    PendingTransaction * result = new PendingTransaction(ptImpl,0);
    return result;
}

void Wallet::createTransactionAsync(const QString &dst_addr, const QString &payment_id,
                               quint64 amount, quint32 mixin_count,
                               PendingTransaction::Priority priority)
{
    QFuture<PendingTransaction*> future = QtConcurrent::run(this, &Wallet::createTransaction,
                                  dst_addr, payment_id,amount, mixin_count, priority);
    QFutureWatcher<PendingTransaction*> * watcher = new QFutureWatcher<PendingTransaction*>();

    connect(watcher, &QFutureWatcher<PendingTransaction*>::finished,
            this, [this, watcher,dst_addr,payment_id,mixin_count]() {
        QFuture<PendingTransaction*> future = watcher->future();
        watcher->deleteLater();
        emit transactionCreated(future.result(),dst_addr,payment_id,mixin_count);
    });
    watcher->setFuture(future);
}

PendingTransaction *Wallet::createTransactionAll(const QString &dst_addr, const QString &payment_id,
                                                 quint32 mixin_count, PendingTransaction::Priority priority)
{
    std::set<uint32_t> subaddr_indices;
    Monero::PendingTransaction * ptImpl = m_walletImpl->createTransaction(
                dst_addr.toStdString(), payment_id.toStdString(), Monero::optional<uint64_t>(), mixin_count,
                static_cast<Monero::PendingTransaction::Priority>(priority), currentSubaddressAccount(), subaddr_indices);
    PendingTransaction * result = new PendingTransaction(ptImpl, this);
    return result;
}

void Wallet::createTransactionAllAsync(const QString &dst_addr, const QString &payment_id,
                               quint32 mixin_count,
                               PendingTransaction::Priority priority)
{
    QFuture<PendingTransaction*> future = QtConcurrent::run(this, &Wallet::createTransactionAll,
                                  dst_addr, payment_id, mixin_count, priority);
    QFutureWatcher<PendingTransaction*> * watcher = new QFutureWatcher<PendingTransaction*>();

    connect(watcher, &QFutureWatcher<PendingTransaction*>::finished,
            this, [this, watcher,dst_addr,payment_id,mixin_count]() {
        QFuture<PendingTransaction*> future = watcher->future();
        watcher->deleteLater();
        emit transactionCreated(future.result(),dst_addr,payment_id,mixin_count);
    });
    watcher->setFuture(future);
}

UnsignedTransaction * Wallet::loadTxFile(const QString &fileName)
{
    qDebug() << "Trying to sign " << fileName;
    Monero::UnsignedTransaction * ptImpl = m_walletImpl->loadUnsignedTx(fileName.toStdString());
    UnsignedTransaction * result = new UnsignedTransaction(ptImpl, m_walletImpl, this);
    return result;
}

bool Wallet::submitTxFile(const QString &fileName) const
{
    qDebug() << "Trying to submit " << fileName;
    if (!m_walletImpl->submitTransaction(fileName.toStdString()))
        return false;
    // import key images
    return m_walletImpl->importKeyImages(fileName.toStdString() + "_keyImages");
}

void Wallet::disposeTransaction(PendingTransaction *t)
{
    m_walletImpl->disposeTransaction(t->m_pimpl);
    delete t;
}

void Wallet::disposeTransaction(UnsignedTransaction *t)
{
    delete t;
}

TransactionHistory *Wallet::history() const
{
    return m_history;
}

TransactionHistorySortFilterModel *Wallet::historyModel() const
{
    if (!m_historyModel) {
        Wallet * w = const_cast<Wallet*>(this);
        m_historyModel = new TransactionHistoryModel(w);
        m_historyModel->setTransactionHistory(this->history());
        m_historySortFilterModel = new TransactionHistorySortFilterModel(w);
        m_historySortFilterModel->setSourceModel(m_historyModel);
    }

    return m_historySortFilterModel;
}

AddressBook *Wallet::addressBook() const
{
    return m_addressBook;
}

AddressBookModel *Wallet::addressBookModel() const
{

    if (!m_addressBookModel) {
        Wallet * w = const_cast<Wallet*>(this);
        m_addressBookModel = new AddressBookModel(w,m_addressBook);
    }

    return m_addressBookModel;
}

Subaddress *Wallet::subaddress()
{
    return m_subaddress;
}

SubaddressModel *Wallet::subaddressModel()
{
    if (!m_subaddressModel) {
        m_subaddressModel = new SubaddressModel(this, m_subaddress);
    }
    return m_subaddressModel;
}

QString Wallet::generatePaymentId() const
{
    return QString::fromStdString(Monero::Wallet::genPaymentId());
}

QString Wallet::integratedAddress(const QString &paymentId) const
{
    return QString::fromStdString(m_walletImpl->integratedAddress(paymentId.toStdString()));
}

QString Wallet::paymentId() const
{
    return m_paymentId;
}

void Wallet::setPaymentId(const QString &paymentId)
{
    m_paymentId = paymentId;
}

bool Wallet::setUserNote(const QString &txid, const QString &note)
{
  return m_walletImpl->setUserNote(txid.toStdString(), note.toStdString());
}

QString Wallet::getUserNote(const QString &txid) const
{
  return QString::fromStdString(m_walletImpl->getUserNote(txid.toStdString()));
}

QString Wallet::getTxKey(const QString &txid) const
{
  return QString::fromStdString(m_walletImpl->getTxKey(txid.toStdString()));
}

QString Wallet::checkTxKey(const QString &txid, const QString &tx_key, const QString &address)
{
    uint64_t received;
    bool in_pool;
    uint64_t confirmations;
    bool success = m_walletImpl->checkTxKey(txid.toStdString(), tx_key.toStdString(), address.toStdString(), received, in_pool, confirmations);
    std::string result = std::string(success ? "true" : "false") + "|" + QString::number(received).toStdString() + "|" + std::string(in_pool ? "true" : "false") + "|" + QString::number(confirmations).toStdString();
    return QString::fromStdString(result);
}

QString Wallet::getTxProof(const QString &txid, const QString &address, const QString &message) const
{
    std::string result = m_walletImpl->getTxProof(txid.toStdString(), address.toStdString(), message.toStdString());
    if (result.empty())
        result = "error|" + m_walletImpl->errorString();
    return QString::fromStdString(result);
}

QString Wallet::checkTxProof(const QString &txid, const QString &address, const QString &message, const QString &signature)
{
    bool good;
    uint64_t received;
    bool in_pool;
    uint64_t confirmations;
    bool success = m_walletImpl->checkTxProof(txid.toStdString(), address.toStdString(), message.toStdString(), signature.toStdString(), good, received, in_pool, confirmations);
    std::string result = std::string(success ? "true" : "false") + "|" + std::string(good ? "true" : "false") + "|" + QString::number(received).toStdString() + "|" + std::string(in_pool ? "true" : "false") + "|" + QString::number(confirmations).toStdString();
    return QString::fromStdString(result);
}

Q_INVOKABLE QString Wallet::getSpendProof(const QString &txid, const QString &message) const
{
    std::string result = m_walletImpl->getSpendProof(txid.toStdString(), message.toStdString());
    if (result.empty())
        result = "error|" + m_walletImpl->errorString();
    return QString::fromStdString(result);
}

Q_INVOKABLE QString Wallet::checkSpendProof(const QString &txid, const QString &message, const QString &signature) const
{
    bool good;
    bool success = m_walletImpl->checkSpendProof(txid.toStdString(), message.toStdString(), signature.toStdString(), good);
    std::string result = std::string(success ? "true" : "false") + "|" + std::string(!success ? m_walletImpl->errorString() : good ? "true" : "false");
    return QString::fromStdString(result);
}

QString Wallet::signMessage(const QString &message, bool filename) const
{
  if (filename) {
    QFile file(message);
    uchar *data = NULL;

    try {
      if (!file.open(QIODevice::ReadOnly))
        return "";
      quint64 size = file.size();
      if (size == 0) {
        file.close();
        return QString::fromStdString(m_walletImpl->signMessage(std::string()));
      }
      data = file.map(0, size);
      if (!data) {
        file.close();
        return "";
      }
      std::string signature = m_walletImpl->signMessage(std::string((const char*)data, size));
      file.unmap(data);
      file.close();
      return QString::fromStdString(signature);
    }
    catch (const std::exception &e) {
      if (data)
        file.unmap(data);
      file.close();
      return "";
    }
  }
  else {
    return QString::fromStdString(m_walletImpl->signMessage(message.toStdString()));
  }
}

bool Wallet::verifySignedMessage(const QString &message, const QString &address, const QString &signature, bool filename) const
{
  if (filename) {
    QFile file(message);
    uchar *data = NULL;

    try {
      if (!file.open(QIODevice::ReadOnly))
        return false;
      quint64 size = file.size();
      if (size == 0) {
        file.close();
        return m_walletImpl->verifySignedMessage(std::string(), address.toStdString(), signature.toStdString());
      }
      data = file.map(0, size);
      if (!data) {
        file.close();
        return false;
      }
      bool ret = m_walletImpl->verifySignedMessage(std::string((const char*)data, size), address.toStdString(), signature.toStdString());
      file.unmap(data);
      file.close();
      return ret;
    }
    catch (const std::exception &e) {
      if (data)
        file.unmap(data);
      file.close();
      return false;
    }
  }
  else {
    return m_walletImpl->verifySignedMessage(message.toStdString(), address.toStdString(), signature.toStdString());
  }
}
bool Wallet::parse_uri(const QString &uri, QString &address, QString &payment_id, uint64_t &amount, QString &tx_description, QString &recipient_name, QVector<QString> &unknown_parameters, QString &error)
{
   std::string s_address, s_payment_id, s_tx_description, s_recipient_name, s_error;
   std::vector<std::string> s_unknown_parameters;
   bool res= m_walletImpl->parse_uri(uri.toStdString(), s_address, s_payment_id, amount, s_tx_description, s_recipient_name, s_unknown_parameters, s_error);
   if(res)
   {
       address = QString::fromStdString(s_address);
       payment_id = QString::fromStdString(s_payment_id);
       tx_description = QString::fromStdString(s_tx_description);
       recipient_name = QString::fromStdString(s_recipient_name);
       for( const auto &p : s_unknown_parameters )
           unknown_parameters.append(QString::fromStdString(p));
   }
   error = QString::fromStdString(s_error);
   return res;
}

bool Wallet::rescanSpent()
{
    return m_walletImpl->rescanSpent();
}

bool Wallet::useForkRules(quint8 required_version, quint64 earlyBlocks) const
{
    if(m_connectionStatus == Wallet::ConnectionStatus_Disconnected)
        return false;
    try {
        return m_walletImpl->useForkRules(required_version,earlyBlocks);
    } catch (const std::exception &e) {
        qDebug() << e.what();
        return false;
    }
}

void Wallet::setWalletCreationHeight(quint64 height)
{
    m_walletImpl->setRefreshFromBlockHeight(height);
    emit walletCreationHeightChanged();
}

QString Wallet::getDaemonLogPath() const
{
    return QString::fromStdString(m_walletImpl->getDefaultDataDir()) + "/blurd.log";
}

QString Wallet::getWalletLogPath() const
{
    const QString filename("blur-gui-wallet.log");

#ifdef Q_OS_MACOS
    return QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0) + "/Library/Logs/" + filename;
#else
    return QCoreApplication::applicationDirPath() + "/" + filename;
#endif
}

bool Wallet::blackballOutput(const QString &pubkey)
{
    QList<QString> list;
    list.push_back(pubkey);
    return blackballOutputs(list, true);
}

bool Wallet::blackballOutputs(const QList<QString> &pubkeys, bool add)
{
    std::vector<std::string> std_pubkeys;
    foreach (const QString &pubkey, pubkeys) {
        std_pubkeys.push_back(pubkey.toStdString());
    }
    return m_walletImpl->blackballOutputs(std_pubkeys, add);
}

bool Wallet::blackballOutputs(const QString &filename, bool add)
{
    QFile file(filename);

    try {
        if (!file.open(QIODevice::ReadOnly))
            return false;
        QList<QString> outputs;
        QTextStream in(&file);
        while (!in.atEnd()) {
            outputs.push_back(in.readLine());
        }
        file.close();
        return blackballOutputs(outputs, add);
    }
    catch (const std::exception &e) {
        file.close();
        return false;
    }
}

bool Wallet::unblackballOutput(const QString &pubkey)
{
    return m_walletImpl->unblackballOutput(pubkey.toStdString());
}

QString Wallet::getRing(const QString &key_image)
{
    std::vector<uint64_t> cring;
    if (!m_walletImpl->getRing(key_image.toStdString(), cring))
        return "";
    QString ring = "";
    for (uint64_t out: cring)
    {
        if (!ring.isEmpty())
            ring = ring + " ";
	QString s;
	s.setNum(out);
        ring = ring + s;
    }
    return ring;
}

QString Wallet::getRings(const QString &txid)
{
    std::vector<std::pair<std::string, std::vector<uint64_t>>> crings;
    if (!m_walletImpl->getRings(txid.toStdString(), crings))
        return "";
    QString ring = "";
    for (const auto &cring: crings)
    {
        if (!ring.isEmpty())
            ring = ring + "|";
        ring = ring + QString::fromStdString(cring.first) + " absolute";
        for (uint64_t out: cring.second)
        {
            ring = ring + " ";
	    QString s;
	    s.setNum(out);
            ring = ring + s;
        }
    }
    return ring;
}

bool Wallet::setRing(const QString &key_image, const QString &ring, bool relative)
{
    std::vector<uint64_t> cring;
    QStringList strOuts = ring.split(" ");
    foreach(QString str, strOuts)
    {
        uint64_t out;
	bool ok;
	out = str.toULong(&ok);
	if (ok)
            cring.push_back(out);
    }
    return m_walletImpl->setRing(key_image.toStdString(), cring, relative);
}

void Wallet::segregatePreForkOutputs(bool segregate)
{
    m_walletImpl->segregatePreForkOutputs(segregate);
}

void Wallet::segregationHeight(quint64 height)
{
    m_walletImpl->segregationHeight(height);
}

void Wallet::keyReuseMitigation2(bool mitigation)
{
    m_walletImpl->keyReuseMitigation2(mitigation);
}

Wallet::Wallet(Monero::Wallet *w, QObject *parent)
    : QObject(parent)
    , m_walletImpl(w)
    , m_history(nullptr)
    , m_historyModel(nullptr)
    , m_addressBook(nullptr)
    , m_addressBookModel(nullptr)
    , m_subaddress(nullptr)
    , m_subaddressModel(nullptr)
    , m_daemonBlockChainHeight(0)
    , m_daemonBlockChainHeightTtl(DAEMON_BLOCKCHAIN_HEIGHT_CACHE_TTL_SECONDS)
    , m_daemonBlockChainTargetHeight(0)
    , m_daemonBlockChainTargetHeightTtl(DAEMON_BLOCKCHAIN_TARGET_HEIGHT_CACHE_TTL_SECONDS)
    , m_connectionStatusTtl(WALLET_CONNECTION_STATUS_CACHE_TTL_SECONDS)
    , m_currentSubaddressAccount(0)
{
    m_history = new TransactionHistory(m_walletImpl->history(), this);
    m_addressBook = new AddressBook(m_walletImpl->addressBook(), this);
    m_subaddress = new Subaddress(m_walletImpl->subaddress(), this);
    m_walletImpl->setListener(new WalletListenerImpl(this));
    m_connectionStatus = Wallet::ConnectionStatus_Disconnected;
    // start cache timers
    m_connectionStatusTime.restart();
    m_daemonBlockChainHeightTime.restart();
    m_daemonBlockChainTargetHeightTime.restart();
    m_initialized = false;
    m_connectionStatusRunning = false;
    m_daemonUsername = "";
    m_daemonPassword = "";
}

Wallet::~Wallet()
{
    qDebug("~Wallet: Closing wallet");
    delete m_addressBook;
    m_addressBook = NULL;

    delete m_history;
    m_history = NULL;
    delete m_addressBook;
    m_addressBook = NULL;
    delete m_subaddress;
    m_subaddress = NULL;
    //Monero::WalletManagerFactory::getWalletManager()->closeWallet(m_walletImpl);
    if(status() == Status_Critical)
        qDebug("Not storing wallet cache");
    else if( m_walletImpl->store(""))
        qDebug("Wallet cache stored successfully");
    else
        qDebug("Error storing wallet cache");
    delete m_walletImpl;
    m_walletImpl = NULL;
    qDebug("m_walletImpl deleted");
}
