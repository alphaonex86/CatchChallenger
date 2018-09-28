#ifndef DATABASEBOT_H
#define DATABASEBOT_H

#include <QSqlDatabase>

class DatabaseBot
{
public:
    explicit DatabaseBot();
    static DatabaseBot databaseBot;
    QSqlDatabase database;
    void init();
};

#endif // DATABASEBOT_H
