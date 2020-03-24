#include "PreparedStatementUnit.hpp"
#if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
#include <postgresql/libpq-fe.h>
#include "../../general/base/cpp11addition.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../epoll/db/EpollPostgresql.hpp"
#include <cstring>
#include <iostream>
#endif

using namespace CatchChallenger;

#if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
std::unordered_map<CatchChallenger::DatabaseBase *,uint16_t> PreparedStatementUnit::queryCount;
#endif

PreparedStatementUnit::PreparedStatementUnit() :
    database(NULL)
{
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    uniqueName[0]=0;
    #endif
}

PreparedStatementUnit::PreparedStatementUnit(const std::string &query, CatchChallenger::DatabaseBase * const database) :
#ifdef CATCHCHALLENGER_CLASS_QT
    database(static_cast<QtDatabase *>(database))
#else
    database(database)
#endif
{
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    uniqueName[0]=0;
    #endif
    if(!query.empty())
        setQuery(query);
}

PreparedStatementUnit::~PreparedStatementUnit()
{
    database=NULL;
}

bool PreparedStatementUnit::setQuery(const std::string &query)
{
    if(query.empty())
        return false;
    if(database==NULL)
        return false;
    this->query=query;
    if(this->query.empty())
        return false;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    if(PreparedStatementUnit::queryCount.find(database)==PreparedStatementUnit::queryCount.cend())
        PreparedStatementUnit::queryCount[database]=0;
    strcpy(uniqueName,std::to_string(PreparedStatementUnit::queryCount.at(database)).c_str());
    PreparedStatementUnit::queryCount[database]++;
    const std::string &newQuery=PreparedStatementUnit::writeToPrepare(query);
    if(!static_cast<EpollPostgresql *>(database)->queryPrepare(uniqueName,newQuery.c_str(),this->query.argumentsCount()/*, NULL*//*paramTypes*/))
    { //if failed quit
        std::cerr << "Problem to prepare the query: " << newQuery << ", error message: " << database->errorMessage() << std::endl;
        abort();
    }
    #endif
    return true;
}

#if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
std::string PreparedStatementUnit::writeToPrepare(const std::string &query)
{
    std::string newQuery=query;
    stringreplaceAll(newQuery,"\"","");
    {
        int8_t index=15;
        while(index>=0)
        {
            const uint8_t &replaceCount=stringreplaceAll(newQuery,"%"+std::to_string(index),"$"+std::to_string(index));
            if(replaceCount>1)
            {
                std::cerr << "Malformed query: " << query << " because have remplaced more than one time the value %" << index << std::endl;
                abort();
            }
            stringreplaceAll(newQuery,"decode('$"+std::to_string(index)+"','hex')","decode($"+std::to_string(index)+",'hex')");
            index--;
        }
    }
    return newQuery;
}
#endif

bool PreparedStatementUnit::empty() const
{
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
    return false;
    #else
    return query.empty();
    #endif
}

std::string PreparedStatementUnit::queryText() const
{
    return query.originalQuery();
}

DatabaseBase::CallBack * PreparedStatementUnit::asyncRead(void * returnObject,CallBackDatabase method,const std::vector<std::string> &values)
{
    if(database==NULL)
        return NULL;
    if(query.empty())
        return NULL;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
            return static_cast<EpollPostgresql *>(database)->asyncPreparedRead(PreparedStatementUnit::writeToPrepare(queryText()),uniqueName,returnObject,method,values);
        #else
            return static_cast<EpollPostgresql *>(database)->asyncPreparedRead("",uniqueName,returnObject,method,values);
        #endif
    #else
    return database->asyncRead(query.compose(values),returnObject,method);
    #endif
}

bool PreparedStatementUnit::asyncWrite(const std::vector<std::string> &values)
{
    if(database==NULL)
        return false;
    if(query.empty())
        return false;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
            return static_cast<EpollPostgresql *>(database)->asyncPreparedWrite(PreparedStatementUnit::writeToPrepare(queryText()),uniqueName,values);
        #else
            return static_cast<EpollPostgresql *>(database)->asyncPreparedWrite("",uniqueName,values);
        #endif
    #else
    return database->asyncWrite(query.compose(values));
    #endif
}

PreparedStatementUnit::PreparedStatementUnit(const PreparedStatementUnit& other) // copy constructor
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    switch(other.database->databaseType())
    {
        default:
            std::cerr << "out of range or wrong type" << std::endl;
            abort();
        break;
        case DatabaseBase::DatabaseType::Mysql:
        case DatabaseBase::DatabaseType::SQLite:
        case DatabaseBase::DatabaseType::PostgreSQL:
        break;
    }
    #endif
    if(other.database==nullptr)
    {
        database=nullptr;
        #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
        this->uniqueName[0]=0x00;
        #endif
        return;
    }
    this->database=other.database;
    query=other.query;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    memcpy(this->uniqueName,other.uniqueName,sizeof(uniqueName));
    #endif
}

PreparedStatementUnit::PreparedStatementUnit(PreparedStatementUnit&& other) // move constructor
    : database(other.database),
      query(other.query)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    switch(other.database->databaseType())
    {
        default:
            std::cerr << "out of range or wrong type" << std::endl;
            abort();
        break;
        case DatabaseBase::DatabaseType::Mysql:
        case DatabaseBase::DatabaseType::SQLite:
        case DatabaseBase::DatabaseType::PostgreSQL:
        break;
    }
    #endif
    other.database = nullptr;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    strcpy(this->uniqueName,other.uniqueName);
    #endif
}

PreparedStatementUnit& PreparedStatementUnit::operator=(const PreparedStatementUnit& other) // copy assignment
{
    if(other.database==nullptr)
    {
        database=nullptr;
        #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
        this->uniqueName[0]=0x00;
        #endif
        return *this;
    }
    this->database=other.database;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    switch(other.database->databaseType())
    {
        default:
            std::cerr << "out of range or wrong type" << std::endl;
            abort();
        break;
        case DatabaseBase::DatabaseType::Mysql:
        case DatabaseBase::DatabaseType::SQLite:
        case DatabaseBase::DatabaseType::PostgreSQL:
        break;
    }
    #endif
    query=other.query;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    memcpy(this->uniqueName,other.uniqueName,sizeof(uniqueName));
    #endif
    return *this;
}

PreparedStatementUnit& PreparedStatementUnit::operator=(PreparedStatementUnit&& other) // move assignment
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    switch(other.database->databaseType())
    {
        default:
            std::cerr << "out of range or wrong type" << std::endl;
            abort();
        break;
        case DatabaseBase::DatabaseType::Mysql:
        case DatabaseBase::DatabaseType::SQLite:
        case DatabaseBase::DatabaseType::PostgreSQL:
        break;
    }
    #endif
    this->database=other.database;
    query=other.query;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    memcpy(this->uniqueName,other.uniqueName,sizeof(uniqueName));
    #endif
    other.database = nullptr;
    return *this;
}
