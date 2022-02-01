#ifndef CATCHCHALLENGER_QtDatabaseMySQL_H
#define CATCHCHALLENGER_QtDatabaseMySQL_H

#include "QtDatabase.hpp"

namespace CatchChallenger {

class QtDatabaseMySQL : public QtDatabase
{
    Q_OBJECT
public:
    QtDatabaseMySQL();
    ~QtDatabaseMySQL();
    bool syncConnect(const std::string &host, const std::string &dbname, const std::string &user, const std::string &password);
};

}

#endif
