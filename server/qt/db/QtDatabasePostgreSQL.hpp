#ifndef CATCHCHALLENGER_QtDatabasePostgreSQL_H
#define CATCHCHALLENGER_QtDatabasePostgreSQL_H

#include "QtDatabase.hpp"

namespace CatchChallenger {

class QtDatabasePostgreSQL : public QtDatabase
{
    Q_OBJECT
public:
    QtDatabasePostgreSQL();
    ~QtDatabasePostgreSQL();
    bool syncConnect(const std::string &host, const std::string &dbname, const std::string &user, const std::string &password);
};

}

#endif
