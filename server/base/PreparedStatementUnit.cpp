#include "PreparedStatementUnit.h"

using namespace CatchChallenger;

#if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
uint16_t PreparedStatementUnit::queryCount=0;
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
    this->query=query;
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
    strcpy(uniqueName,std::to_string(PreparedStatementUnit::queryCount).c_str());
    PreparedStatementUnit::queryCount++;
    std::string newQuery=query;
    newQuery.replace("\"","");
    {
        int8_t index=15;
        while(index>=0)
        {
            newQuery.replace("%"+std::to_string(index),"?");
            index--;
        }
    }
    PGresult *resprep = PQprepare(database,uniqueName,newQuery.c_str(), 1, NULL/*paramTypes*/);
    if (PQresultStatus(resprep) != PGRES_COMMAND_OK)
    { //if failed quit
        std::cerr << "Problem to prepare the query: " << newQuery << std::endl;
        abort();
    }
    #endif
}

bool PreparedStatementUnit::empty() const
{
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
    return false;
    #else
    return query.empty();
    #endif
}

const std::string &PreparedStatementUnit::queryText() const
{
    return query;
}

CallBack * PreparedStatementUnit::asyncRead(void * returnObject,CallBackDatabase method,const std::vector<std::string> &values)
{
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
    return database->asyncPreparedRead(uniqueName,returnObject,method,values);
    #else
    return database->asyncRead(query.compose(values),returnObject,method);
    #endif
}

bool PreparedStatementUnit::asyncWrite(const std::vector<std::string> &values)
{
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
    return database->asyncPreparedWrite(uniqueName,values);
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
    : preparedQuery(other.preparedQuery)
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
