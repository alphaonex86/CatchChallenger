#ifndef CATCHCHALLENGER_QtDatabaseSQLite_H
#define CATCHCHALLENGER_QtDatabaseSQLite_H

#include "QtDatabase.hpp"

namespace CatchChallenger {

class QtDatabaseSQLite : public QtDatabase
{
    Q_OBJECT
public:
    QtDatabaseSQLite();
    ~QtDatabaseSQLite();
    bool syncConnect(const std::string &host, const std::string &dbname, const std::string &user, const std::string &password);
};

}

#endif
