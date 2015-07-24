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
    void receiveQuery(const QString &query, const QSqlDatabase &db);
signals:
    void sendReply(const QSqlQuery &queryReturn);
};

class QtDatabase : public QObject, public CatchChallenger::DatabaseBase
{
    Q_OBJECT
public:
    QtDatabase();
    ~QtDatabase();
    bool syncConnect(const char * host, const char * dbname, const char * user, const char * password);
    bool syncConnectMysql(const char * host, const char * dbname, const char * user, const char * password);
    bool syncConnectSqlite(const char * file);
    bool syncConnectPostgresql(const char * host, const char * dbname, const char * user, const char * password);
    void syncDisconnect();
    CallBack * asyncRead(const char *query,void * returnObject,CallBackDatabase method);
    bool asyncWrite(const char *query);
    bool readyToRead();
    void clear();
    const char * errorMessage() const;
    bool next();
    const char * value(const int &value) const;
    bool isConnected() const;
    void receiveReply(const QSqlQuery &queryReturn);
    DatabaseBase::DatabaseType databaseType() const;
    bool epollEvent(const uint32_t &events);

    QtDatabaseThread dbThread;
signals:
    void sendQuery(const QString &query, const QSqlDatabase &db);
private:
    QSqlDatabase *conn;
    QSqlQuery *sqlQuery;
    static char emptyString[1];
    QString lastErrorMessage;
    QList<CallBack> queue;
    QList<QString> queriesList;
    static QByteArray valueReturnedData;
    DatabaseBase::DatabaseType databaseConnected;
};

}

#endif
