#ifndef CATCHCHALLENGER_QTDATABASE_H
#define CATCHCHALLENGER_QTDATABASE_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QThread>
#include <QObject>
#include <atomic>
#include <cstdint>

#define CATCHCHALLENGER_MAXBDQUERIES 255

#include "../../base/DatabaseBase.hpp"

namespace CatchChallenger {

#ifdef CATCHCHALLENGER_CLASS_QT
// Live SQL stats (GUI dashboard probe). Increment on every asyncRead/
// asyncWrite/syncWrite/syncRead and on every reply received. The
// MainWindow polls these counters once per second + computes deltas
// for the "DB query" KPI tile and "SQL latency" gauge.
//
// Atomic + process-wide so the counters are correct across the
// QtDatabaseThread that actually runs the queries. They're declared
// here (header) and defined in QtDatabase.cpp; gated on the
// CATCHCHALLENGER_CLASS_QT define so the headless unix/io_uring
// build never sees the symbols.
extern std::atomic<uint64_t> qtDbStats_queryTotalCount;
extern std::atomic<uint64_t> qtDbStats_replyTotalCount;
extern std::atomic<uint64_t> qtDbStats_lastQueryDurationNs;
#endif

class QtDatabaseThread : public QThread
{
    Q_OBJECT
public:
    explicit QtDatabaseThread();
    ~QtDatabaseThread();
    void receiveQuery(const std::string &query, const QSqlDatabase &db);
signals:
    void sendReply(const QSqlQuery &queryReturn);
};

class QtDatabase : public QObject, public CatchChallenger::DatabaseBase
{
    Q_OBJECT
public:
    QtDatabase();
    ~QtDatabase();
    void syncDisconnect();
    DatabaseBaseCallBack * asyncRead(const std::string &query,void * returnObject,CallBackDatabase method);
    bool asyncWrite(const std::string &query);
    bool readyToRead();
    void clear();
    const std::string errorMessage() const;
    bool next();
    const std::string value(const int &value) const;
    bool isConnected() const;
    void receiveReply(const QSqlQuery &queryReturn);
    bool unixEvent(const uint32_t &events);

    QtDatabaseThread dbThread;

    struct EstablishedConnexion
    {
        std::string host;
        std::string dbname;
        QSqlDatabase *conn;
        unsigned int openCount;
        DatabaseBase::DatabaseType type;
    };
    static std::vector<EstablishedConnexion> establishedConnexionList;
    static unsigned int establishedConnexionCount;

    static QSqlDatabase * findConnexionToOpen(const std::string &host, const std::string &dbname, const DatabaseType &typeToSearch);
    static unsigned int findConnexionToClose(const QSqlDatabase * const conn);
signals:
    void sendQuery(const std::string &query, const QSqlDatabase &db);
protected:
    QSqlDatabase *conn;
    QSqlQuery *sqlQuery;
    static char emptyString[1];
    std::string lastErrorMessage;
    std::vector<DatabaseBaseCallBack> queue;
    std::vector<std::string> queriesList;
};

}

#endif
