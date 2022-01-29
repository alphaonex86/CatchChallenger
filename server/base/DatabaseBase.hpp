#ifndef DATABASEBASE_H
#define DATABASEBASE_H

#include <stdint.h>
#include <string>
#include "DatabaseFunction.hpp"

typedef void (*CallBackDatabase)(void *object);

namespace CatchChallenger {
class DatabaseBaseCallBack
{
    public:
    void *object;
    CallBackDatabase method;
};
#if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
class DatabaseBaseWithPreparedQuery
{
    public:
    virtual DatabaseBaseCallBack * asyncPreparedRead(const std::string &query,char * const id,void * returnObject,CallBackDatabase method,const std::vector<std::string> &values) = 0;
    virtual bool asyncPreparedWrite(const std::string &query,char * const id,const std::vector<std::string> &values) = 0;
    virtual bool queryPrepare(const char *stmtName,const char *query,const int &nParams,const bool &store=true) = 0;//return NULL if failed
};
#endif
class DatabaseBase : public CatchChallenger::DatabaseFunction
        #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
        , public DatabaseBaseWithPreparedQuery
        #endif
{
    public:
        enum DatabaseType : uint8_t
        {
            Unknown=0x00,
            #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_CLASS_QT)
            Mysql=0x01,
            #endif
            #if defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_CLASS_QT)
            SQLite=0x02,
            #endif
            #if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
            PostgreSQL=0x03,
            #endif
        };
        DatabaseBase();
        virtual ~DatabaseBase();
        virtual bool syncConnect(const std::string &host, const std::string &dbname, const std::string &user, const std::string &password) = 0;
        virtual void syncDisconnect() = 0;
        virtual DatabaseBaseCallBack * asyncRead(const std::string &query,void * returnObject,CallBackDatabase method) = 0;
        virtual bool asyncWrite(const std::string &query) = 0;
        virtual const std::string errorMessage() const = 0;
        virtual bool next() = 0;
        virtual const std::string value(const int &value) const = 0;
        virtual bool isConnected() const = 0;
        virtual bool epollEvent(const uint32_t &events) = 0;
        //sync mode then prefer tryInterval*considerDownAfterNumberOfTry < 20s
        unsigned int tryInterval;//second
        unsigned int considerDownAfterNumberOfTry;
        DatabaseType databaseType() const;
        virtual void clear();
        static const std::string databaseTypeToString(const DatabaseType &type);
        virtual bool setBlocking(const bool &val);//return true if success
        virtual bool setMaxDbQueries(const unsigned int &maxDbQueries);
    protected:
        DatabaseType databaseTypeVar;//to def at runtime the value, used mostly for Qt database
};
}

#endif // DATABASEBASE_H
