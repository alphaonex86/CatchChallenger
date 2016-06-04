#ifndef CATCHCHALLENGER_QTDATABASE_H
#define CATCHCHALLENGER_QTDATABASE_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QThread>
#include <QObject>

#define CATCHCHALLENGER_MAXBDQUERIES 255

#include "DatabaseBase.h"

namespace CatchChallenger {

class QtDatabaseThread : public QThread
{
    Q_OBJECT
public:
    QtDatabaseThread();
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
    bool syncConnect(const std::string &host, const std::string &dbname, const std::string &user, const std::string &password);
    bool syncConnectMysql(const std::string &host, const std::string &dbname, const std::string &user, const std::string &password);
    bool syncConnectSqlite(const std::string &file);
    bool syncConnectPostgresql(const std::string &host, const std::string &dbname, const std::string &user, const std::string &password);
    void syncDisconnect();
    CallBack * asyncRead(const std::string &query,void * returnObject,CallBackDatabase method);
    bool asyncWrite(const std::string &query);
    bool readyToRead();
    void clear();
    const std::string errorMessage() const;
    bool next();
    const std::string value(const int &value) const;
    bool isConnected() const;
    void receiveReply(const QSqlQuery &queryReturn);
    DatabaseBase::DatabaseType databaseType() const;
    bool epollEvent(const uint32_t &events);

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
private:
    QSqlDatabase *conn;
    QSqlQuery *sqlQuery;
    static char emptyString[1];
    std::string lastErrorMessage;
    std::vector<CallBack> queue;
    std::vector<std::string> queriesList;
    DatabaseBase::DatabaseType databaseConnected;
};

}

#endif
