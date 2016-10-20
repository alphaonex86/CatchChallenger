#include "PreparedStatementUnit.h"
#if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
#include <postgresql/libpq-fe.h>
#include "../../general/base/cpp11addition.h"
#include "../epoll/db/EpollPostgresql.h"
#include <cstring>
#include <iostream>
#endif

using namespace CatchChallenger;

#if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
std::unordered_map<CatchChallenger::DatabaseBase *,uint16_t> PreparedStatementUnit::queryCount;
#endif

PreparedStatementUnit::PreparedStatementUnit() :
    database(NULL)
{
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
    uniqueName[0]=0;
    #endif
}

PreparedStatementUnit::PreparedStatementUnit(const std::string &query, CatchChallenger::DatabaseBase * const database) :
    database(database)
{
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
    uniqueName[0]=0;
    #endif
    if(!query.empty())
        setQuery(query);
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
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
    if(PreparedStatementUnit::queryCount.find(database)==PreparedStatementUnit::queryCount.cend())
        PreparedStatementUnit::queryCount[database]=0;
    strcpy(uniqueName,std::to_string(PreparedStatementUnit::queryCount.at(database)).c_str());
    PreparedStatementUnit::queryCount[database]++;
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
    //stringreplaceAll(newQuery,"decode('?','hex')","?");
    if(!static_cast<EpollPostgresql *>(database)->queryPrepare(uniqueName,newQuery.c_str(),this->query.argumentsCount(), NULL/*paramTypes*/))
    { //if failed quit
        std::cerr << "Problem to prepare the query: " << newQuery << ", error message: " << database->errorMessage() << std::endl;
        abort();
    }
    #endif
    return true;
}

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
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
    return static_cast<EpollPostgresql *>(database)->asyncPreparedRead(queryText(),uniqueName,returnObject,method,values);
    #else
    return database->asyncRead(query.compose(values),returnObject,method);
    #endif
}

bool PreparedStatementUnit::asyncWrite(const std::vector<std::string> &values)
{
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
    return static_cast<EpollPostgresql *>(database)->asyncPreparedWrite(queryText(),uniqueName,values);
    #else
    return database->asyncWrite(query.compose(values));
    #endif
}

PreparedStatementUnit::PreparedStatementUnit(const PreparedStatementUnit& other) // copy constructor
{
    if(other.database==nullptr)
    {
        database=nullptr;
        return;
    }
    this->database=other.database;
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
    memcpy(this->uniqueName,other.uniqueName,sizeof(uniqueName));
    #else
    setQuery(other.query);
    #endif
}

PreparedStatementUnit::PreparedStatementUnit(PreparedStatementUnit&& other) // move constructor
    : query(other.query)
{
    other.database = nullptr;
}

PreparedStatementUnit& PreparedStatementUnit::operator=(const PreparedStatementUnit& other) // copy assignment
{
    if(other.database==nullptr)
    {
        database=nullptr;
        return *this;
    }
    this->database=other.database;
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
    memcpy(this->uniqueName,other.uniqueName,sizeof(uniqueName));
    #else
    setQuery(other.query);
    #endif
    return *this;
}

PreparedStatementUnit& PreparedStatementUnit::operator=(PreparedStatementUnit&& other) // move assignment
{
    this->database=other.database;
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
    memcpy(this->uniqueName,other.uniqueName,sizeof(uniqueName));
    #else
    setQuery(other.query);
    #endif
    other.database = nullptr;
    return *this;
}
