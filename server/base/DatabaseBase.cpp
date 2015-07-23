#include "DatabaseBase.h"
#include <iostream>

using namespace CatchChallenger;

DatabaseBase::DatabaseBase() :
    tryInterval(1),
    considerDownAfterNumberOfTry(30),
    databaseTypeVar(DatabaseBase::Type::Unknown)
{
}

void DatabaseBase::clear()
{
}

const char * DatabaseBase::databaseTypeToString(const DatabaseBase::Type &type)
{
    switch(type)
    {
        default:
            return "Unknown";
        break;
        case DatabaseBase::Type::Mysql:
            return "Mysql";
        break;
        case DatabaseBase::Type::SQLite:
            return "SQLite";
        break;
        case DatabaseBase::Type::PostgreSQL:
            return "PostgreSQL";
        break;
    }
}

DatabaseBase::Type DatabaseBase::databaseType() const
{
    std::cout << "databaseTypeVar " << databaseTypeVar << std::endl;
    if(databaseTypeVar==Mysql)
        abort();
    return databaseTypeVar;
}
