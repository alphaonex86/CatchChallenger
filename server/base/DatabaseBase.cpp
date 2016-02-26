#include "DatabaseBase.h"
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

void DatabaseBase::clear()
{
}

const std::string DatabaseBase::databaseTypeToString(const DatabaseBase::DatabaseType &type)
{
    switch(type)
    {
        default:
            return "Unknown";
        break;
        case DatabaseBase::DatabaseType::Mysql:
            return "Mysql";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            return "SQLite";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            return "PostgreSQL";
        break;
    }
}

DatabaseBase::DatabaseType DatabaseBase::databaseType() const
{
    return databaseTypeVar;
}

BaseClassSwitch::EpollObjectType DatabaseBase::getType() const
{
    return BaseClassSwitch::EpollObjectType::Database;
}
