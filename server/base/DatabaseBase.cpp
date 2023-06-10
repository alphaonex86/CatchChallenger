#include "DatabaseBase.hpp"
#include <iostream>

using namespace CatchChallenger;

DatabaseBase::DatabaseBase() :
    tryInterval(1),
    considerDownAfterNumberOfTry(30),
    databaseTypeVar(DatabaseBase::DatabaseType::Unknown)
{
}

DatabaseBase::~DatabaseBase()
{
}

#ifndef CATCHCHALLENGER_DB_BLACKHOLE
void DatabaseBase::clear()
{
}
#endif

const std::string DatabaseBase::databaseTypeToString(const DatabaseBase::DatabaseType &type)
{
    switch(type)
    {
        default:
            return "Unknown ("+std::to_string((int)type)+")";
        break;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::Mysql:
            return "Mysql";
        break;
        #endif
        #if defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::SQLite:
            return "SQLite";
        break;
        #endif
        #if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case DatabaseBase::DatabaseType::PostgreSQL:
            return "PostgreSQL";
        break;
        #endif
        #ifdef CATCHCHALLENGER_DB_BLACKHOLE
        case DatabaseBase::DatabaseType::BlackHole:
            return "BlackHole";
        break;
        #endif
    }
}

DatabaseBase::DatabaseType DatabaseBase::databaseType() const
{
    return databaseTypeVar;
}

bool DatabaseBase::setBlocking(const bool &val)//return true if success
{
    (void)val;
    return true;
}

bool DatabaseBase::setMaxDbQueries(const unsigned int &maxDbQueries)
{
    (void)maxDbQueries;
    return true;
}
