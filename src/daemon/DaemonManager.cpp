#include "DaemonManager.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QUrl>
#include <QtConcurrent/QtConcurrent>
#include <QApplication>
#include <QProcess>
#include <QTime>
#include <QStorageInfo>
#include <QVariantMap>
#include <QVariant>
#include <QMap>

namespace {
    static const int DAEMON_START_TIMEOUT_SECONDS = 30;
}

DaemonManager * DaemonManager::m_instance = nullptr;
QStringList DaemonManager::m_clArgs;

DaemonManager *DaemonManager::instance(const QStringList *args)
{
    if (!m_instance) {
        m_instance = new DaemonManager;
        // store command line arguments for later use
        m_clArgs = *args;
        m_clArgs.removeFirst();
    }

    return m_instance;
}

bool DaemonManager::start(const QString &flags, NetworkType::Type nettype, const QString &dataDir, const QString &bootstrapNodeAddress)
{
    // prepare command line arguments and pass to blurd
    QStringList arguments;

    // Start daemon with --detach flag on non-windows platforms
#ifndef Q_OS_WIN
    arguments << "--detach";
#endif

    if (nettype == NetworkType::TESTNET)
        arguments << "--testnet";
    else if (nettype == NetworkType::STAGENET)
        arguments << "--stagenet";

    foreach (const QString &str, m_clArgs) {
          qDebug() << QString(" [%1] ").arg(str);
          if (!str.isEmpty())
            arguments << str;
    }

    // Custom startup flags for daemon
    foreach (const QString &str, flags.split(" ")) {
          qDebug() << QString(" [%1] ").arg(str);
          if (!str.isEmpty())
            arguments << str;
    }

    // Custom data-dir
    if(!dataDir.isEmpty()) {
        arguments << "--data-dir" << dataDir;
    }

    // Bootstrap node address
    if(!bootstrapNodeAddress.isEmpty()) {
        arguments << "--bootstrap-daemon-address" << bootstrapNodeAddress;
    }


    qDebug() << "starting blurd " + m_blurd;
    qDebug() << "With command line arguments " << arguments;

    m_daemon = new QProcess();
    initialized = true;

    // Connect output slots
    connect (m_daemon, SIGNAL(readyReadStandardOutput()), this, SLOT(printOutput()));
    connect (m_daemon, SIGNAL(readyReadStandardError()), this, SLOT(printError()));

    // Start blurd
    bool started = m_daemon->startDetached(m_blurd, arguments);

    // add state changed listener
    connect(m_daemon,SIGNAL(stateChanged(QProcess::ProcessState)),this,SLOT(stateChanged(QProcess::ProcessState)));

    if (!started) {
        qDebug() << "Daemon start error: " + m_daemon->errorString();
        emit daemonStartFailure();
        return false;
    }

    // Start start watcher
    QFuture<bool> future = QtConcurrent::run(this, &DaemonManager::startWatcher, nettype);
    QFutureWatcher<bool> * watcher = new QFutureWatcher<bool>();
    connect(watcher, &QFutureWatcher<bool>::finished,
            this, [this, watcher]() {
        QFuture<bool> future = watcher->future();
        watcher->deleteLater();
        if(future.result())
            emit daemonStarted();
        else
            emit daemonStartFailure();
    });
    watcher->setFuture(future);


    return true;
}

bool DaemonManager::stop(NetworkType::Type nettype)
{
    QString message;
    sendCommand("exit", nettype, message);
    qDebug() << message;

    // Start stop watcher - Will kill if not shutting down
    QFuture<bool> future = QtConcurrent::run(this, &DaemonManager::stopWatcher, nettype);
    QFutureWatcher<bool> * watcher = new QFutureWatcher<bool>();
    connect(watcher, &QFutureWatcher<bool>::finished,
            this, [this, watcher]() {
        QFuture<bool> future = watcher->future();
        watcher->deleteLater();
        if(future.result()) {
            emit daemonStopped();
        }
    });
    watcher->setFuture(future);

    return true;
}

bool DaemonManager::startWatcher(NetworkType::Type nettype) const
{
    // Check if daemon is started every 2 seconds
    QTime timer;
    timer.restart();
    while(true && !m_app_exit && timer.elapsed() / 1000 < DAEMON_START_TIMEOUT_SECONDS  ) {
        QThread::sleep(2);
        if(!running(nettype)) {
            qDebug() << "daemon not running. checking again in 2 seconds.";
        } else {
            qDebug() << "daemon is started. Waiting 5 seconds to let daemon catch up";
            QThread::sleep(5);
            return true;
        }
    }
    return false;
}

bool DaemonManager::stopWatcher(NetworkType::Type nettype) const
{
    // Check if daemon is running every 2 seconds. Kill if still running after 10 seconds
    int counter = 0;
    while(true && !m_app_exit) {
        QThread::sleep(2);
        counter++;
        if(running(nettype)) {
            qDebug() << "Daemon still running.  " << counter;
            if(counter >= 5) {
                qDebug() << "Killing it! ";
#ifdef Q_OS_WIN
                QProcess::execute("taskkill /F /IM blurd.exe");
#else
                QProcess::execute("pkill blurd");
#endif
            }

        } else
            return true;
    }
    return false;
}


void DaemonManager::stateChanged(QProcess::ProcessState state)
{
    qDebug() << "STATE CHANGED: " << state;
    if (state == QProcess::NotRunning) {
        emit daemonStopped();
    }
}

void DaemonManager::printOutput()
{
    QByteArray byteArray = m_daemon->readAllStandardOutput();
    QStringList strLines = QString(byteArray).split("\n");

    foreach (QString line, strLines) {
        emit daemonConsoleUpdated(line);
        qDebug() << "Daemon: " + line;
    }
}

void DaemonManager::printError()
{
    QByteArray byteArray = m_daemon->readAllStandardError();
    QStringList strLines = QString(byteArray).split("\n");

    foreach (QString line, strLines) {
        emit daemonConsoleUpdated(line);
        qDebug() << "Daemon ERROR: " + line;
    }
}

bool DaemonManager::running(NetworkType::Type nettype) const
{ 
    QString status;
    sendCommand("status", nettype, status);
    qDebug() << status;
    // `./blurd status` returns BUSY when syncing.
    // Treat busy as connected, until fixed upstream.
    if (status.contains("Height:") || status.contains("BUSY") ) {
        return true;
    }
    return false;
}
bool DaemonManager::sendCommand(const QString &cmd, NetworkType::Type nettype) const
{
    QString message;
    return sendCommand(cmd, nettype, message);
}

bool DaemonManager::sendCommand(const QString &cmd, NetworkType::Type nettype, QString &message) const
{
    QProcess p;
    QStringList external_cmd;
    external_cmd << cmd;

    // Add network type flag if needed
    if (nettype == NetworkType::TESTNET)
        external_cmd << "--testnet";
    else if (nettype == NetworkType::STAGENET)
        external_cmd << "--stagenet";

    qDebug() << "sending external cmd: " << external_cmd;


    p.start(m_blurd, external_cmd);

    bool started = p.waitForFinished(-1);
    message = p.readAllStandardOutput();
    emit daemonConsoleUpdated(message);
    return started;
}

void DaemonManager::exit()
{
    qDebug("DaemonManager: exit()");
    m_app_exit = true;
}

QVariantMap DaemonManager::validateDataDir(const QString &dataDir) const
{
    QVariantMap result;
    bool valid = true;
    bool readOnly = false;
    int  storageAvailable = 0;
    bool lmdbExists = true;

    QStorageInfo storage(dataDir);
    if (storage.isValid() && storage.isReady()) {
        if (storage.isReadOnly()) {
            readOnly = true;
            valid = false;
        }

        // Make sure there is 20GB storage available
        storageAvailable = storage.bytesAvailable()/1000/1000/1000;
        if (storageAvailable < 20) {
            valid = false;
        }
    } else {
        valid = false;
    }


    if (!QDir(dataDir+"/lmdb").exists()) {
        lmdbExists = false;
        valid = false;
    }

    result.insert("valid", valid);
    result.insert("lmdbExists", lmdbExists);
    result.insert("readOnly", readOnly);
    result.insert("storageAvailable", storageAvailable);

    return result;
}

DaemonManager::DaemonManager(QObject *parent)
    : QObject(parent)
{

    // Platform depetent path to blurd
#ifdef Q_OS_WIN
    m_blurd = QApplication::applicationDirPath() + "/blurd.exe";
#elif defined(Q_OS_UNIX)
    m_blurd = QApplication::applicationDirPath() + "/blurd";
#endif

    if (m_blurd.length() == 0) {
        qCritical() << "no daemon binary defined for current platform";
        m_has_daemon = false;
    }
}
