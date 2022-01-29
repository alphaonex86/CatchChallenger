#include "PreparedStatementUnit.hpp"
#if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
#include <postgresql/libpq-fe.h>
#include "../../general/base/cpp11addition.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include <cstring>
#endif
#include <iostream>

using namespace CatchChallenger;

#if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
std::unordered_map<CatchChallenger::DatabaseBase *,uint16_t> PreparedStatementUnit::queryCount;
#endif

PreparedStatementUnit::PreparedStatementUnit()
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    : database(NULL)
    #endif
{
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    uniqueName[0]=0;
    #endif
}

PreparedStatementUnit::PreparedStatementUnit(const std::string &query, CatchChallenger::DatabaseBase * const database)
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    : database(database)
    #endif
{
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    uniqueName[0]=0;
    #else
    (void)database;
    #endif
    if(!query.empty())
        setQuery(query);
}

PreparedStatementUnit::~PreparedStatementUnit()
{
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    database=NULL;
    #endif
}

bool PreparedStatementUnit::setQuery(const std::string &query)
{
    if(query.empty())
        return false;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    if(database==NULL)
        return false;
    #endif
    this->query=query;
    if(this->query.empty())
        return false;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    if(PreparedStatementUnit::queryCount.find(database)==PreparedStatementUnit::queryCount.cend())
        PreparedStatementUnit::queryCount[database]=0;
    strcpy(uniqueName,std::to_string(PreparedStatementUnit::queryCount.at(database)).c_str());
    PreparedStatementUnit::queryCount[database]++;
    const std::string &newQuery=PreparedStatementUnit::writeToPrepare(query);
    if(!database->queryPrepare(uniqueName,newQuery.c_str(),this->query.argumentsCount()/*, NULL*//*paramTypes*/))
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

DatabaseBaseCallBack *PreparedStatementUnit::asyncRead(void * returnObject,CallBackDatabase method,const std::vector<std::string> &values)
{
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    if(database==NULL)
        return NULL;
    #else
    (void)returnObject;
    (void)method;
    (void)values;
    #endif
    if(query.empty())
        return NULL;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
            return database->asyncPreparedRead(PreparedStatementUnit::writeToPrepare(queryText()),uniqueName,returnObject,method,values);
        #else
            return database->asyncPreparedRead("",uniqueName,returnObject,method,values);
        #endif
    #else
        #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
        return database->asyncRead(query.compose(values),returnObject,method);
        #else
        std::cerr << "PreparedStatementUnit::asyncRead call when disabled" << std::endl;
        abort();
        #endif
    #endif
}

bool PreparedStatementUnit::asyncWrite(const std::vector<std::string> &values)
{
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    if(database==NULL)
        return false;
    #else
    (void)values;
    #endif
    if(query.empty())
        return false;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
            return database->asyncPreparedWrite(PreparedStatementUnit::writeToPrepare(queryText()),uniqueName,values);
        #else
            return database->asyncPreparedWrite("",uniqueName,values);
        #endif
    #else
        #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
            return database->asyncWrite(query.compose(values));
        #else
            std::cerr << "PreparedStatementUnit::asyncWrite call when disabled" << std::endl;
            abort();
        #endif
    #endif
}

PreparedStatementUnit::PreparedStatementUnit(const PreparedStatementUnit& other) // copy constructor
{
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    switch(other.database->databaseType())
    {
        default:
            std::cerr << "out of range or wrong type" << std::endl;
            abort();
        break;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::Mysql:
        #endif
        #if defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::SQLite:
        #endif
        #if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::PostgreSQL:
        #endif
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
    #else
    (void)other;
    #endif
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    memcpy(this->uniqueName,other.uniqueName,sizeof(uniqueName));
    #endif
}

PreparedStatementUnit::PreparedStatementUnit(PreparedStatementUnit&& other) : // move constructor
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    database(other.database),
      #endif
      query(other.query)
{
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    switch(other.database->databaseType())
    {
        default:
            std::cerr << "out of range or wrong type" << std::endl;
            abort();
        break;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::Mysql:
        #endif
        #if defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::SQLite:
        #endif
        #if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::PostgreSQL:
        #endif
        break;
    }
    #endif
    other.database = nullptr;
    strcpy(this->uniqueName,other.uniqueName);
    #else
    (void)other;
    #endif
}

PreparedStatementUnit& PreparedStatementUnit::operator=(const PreparedStatementUnit& other) // copy assignment
{
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
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
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::Mysql:
        #endif
        #if defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::SQLite:
        #endif
        #if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::PostgreSQL:
        #endif
        break;
    }
    #endif
    #endif
    query=other.query;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    memcpy(this->uniqueName,other.uniqueName,sizeof(uniqueName));
    #else
    (void)other;
    #endif
    return *this;
}

PreparedStatementUnit& PreparedStatementUnit::operator=(PreparedStatementUnit&& other) // move assignment
{
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    switch(other.database->databaseType())
    {
        default:
            std::cerr << "out of range or wrong type" << std::endl;
            abort();
        break;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::Mysql:
        #endif
        #if defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::SQLite:
        #endif
        #if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::PostgreSQL:
        #endif
        break;
    }
    #endif
    this->database=other.database;
    #endif
    query=other.query;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    memcpy(this->uniqueName,other.uniqueName,sizeof(uniqueName));
    other.database = nullptr;
    #else
    (void)other;
    #endif
    return *this;
}
