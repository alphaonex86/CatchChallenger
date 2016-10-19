#include "PreparedStatementUnit.h"

using namespace CatchChallenger;

PreparedStatementUnit::PreparedStatementUnit() :
    database(NULL)
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
    ,uniqueName("")
    #endif
{
}

PreparedStatementUnit::PreparedStatementUnit(const std::string &query, void * const database) :
    database(database)
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
    ,uniqueName("")
    #endif
{
    if(!query.empty())
        setQuery(query);
}

bool PreparedStatementUnit::setQuery(const std::string &query)
{
    if(query.empty())
        return false;
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
    convert query %1 into ?
                remove "
    PGresult *resprep = PQprepare(conn, "testprepared",query.c_str(), 1, NULL/*paramTypes*/);
    if (PQresultStatus(resprep) != PGRES_COMMAND_OK)
    { //if failed quit
        printf("Problem to prepare it\n");
        PQclear(resprep);
        do_exit(conn);
    }
    char uniqueName[3];
    #else
    this->query=query;
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
