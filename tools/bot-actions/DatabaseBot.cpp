#include "DatabaseBot.h"

#include <QString>
#include <QCoreApplication>
#include <QFile>

#include <iostream>

DatabaseBot DatabaseBot::databaseBot;

DatabaseBot::DatabaseBot()
{
    const QString destination("config.db");
    QFile destinationFile(destination);
    if(!destinationFile.exists())
    {
        QFile::copy(":/config.db",destination);
        destinationFile.setPermissions(destinationFile.permissions() | QFileDevice::WriteOwner | QFileDevice::WriteUser);
    }

    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(destination);

    if (!database.open())
    {
        std::cerr << "Error: connection with database fail" << std::endl;
        abort();
    }
}
