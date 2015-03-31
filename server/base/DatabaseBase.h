#ifndef DATABASEBASE_H
#define DATABASEBASE_H

typedef void (*CallBackDatabase)(void *object);

namespace CatchChallenger {
class DatabaseBase
{
    public:
        enum Type
        {
            Mysql=0x01,
            SQLite=0x02,
            PostgreSQL=0x03
        };
        struct CallBack
        {
            void *object;
            CallBackDatabase method;
        };
        DatabaseBase();
        virtual bool syncConnect(const char * host, const char * dbname, const char * user, const char * password) = 0;
        virtual void syncDisconnect() = 0;
        virtual CallBack * asyncRead(const char *query,void * returnObject,CallBackDatabase method) = 0;
        virtual bool asyncWrite(const char *query) = 0;
        virtual const char * errorMessage() const = 0;
        virtual bool next() = 0;
        virtual const char * value(const int &value) const = 0;
        virtual bool isConnected() const = 0;
        //sync mode then prefer tryInterval*considerDownAfterNumberOfTry < 20s
        unsigned int tryInterval;//second
        unsigned int considerDownAfterNumberOfTry;
        virtual DatabaseBase::Type databaseType() const = 0;
        virtual void clear();
};
}

#endif // DATABASEBASE_H
