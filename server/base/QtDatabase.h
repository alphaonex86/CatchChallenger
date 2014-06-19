#ifndef CATCHCHALLENGER_QTDATABASE_H
#define CATCHCHALLENGER_QTDATABASE_H

#include <QSqlDatabase>
#include <QSqlQuery>

#include "DatabaseBase.h"

namespace CatchChallenger {
class QtDatabase
{
public:
    QtDatabase();
    ~QtDatabase();
    bool syncConnect(const char * host, const char * dbname, const char * user, const char * password);
    bool syncConnectMysql(const char * host, const char * dbname, const char * user, const char * password);
    bool syncConnectSqlite(const char * file);
    bool syncConnectPostgresql(const char * host, const char * dbname, const char * user, const char * password);
    void syncDisconnect();
    bool asyncRead(const char *query,void * returnObject,CallBackDatabase method);
    bool asyncWrite(const char *query);
    bool readyToRead();
    void clear();
    char * errorMessage();
    bool next();
    char * value(const int &value);
    bool isConnected() const;
private:
    QSqlDatabase *conn;
    QSqlQuery *sqlQuery;
    static char emptyString[1];
};
}

#endif
