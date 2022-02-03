#include "DatabaseBot.h"

#include <QString>
#include <QCoreApplication>
#include <QFile>

#include <iostream>

DatabaseBot DatabaseBot::databaseBot;

DatabaseBot::DatabaseBot()
{
}

void DatabaseBot::init()
{
    const QString destination("config.db");
    QFile destinationFile(destination);
    if(!destinationFile.exists())
    {
        QFile sourceFile(":/config.db");
        if(!sourceFile.exists())
            abort();
        else
        {
            if(sourceFile.open(QIODevice::ReadOnly))
            {
                if(sourceFile.size()<1024)
                {
                    std::cerr << "destination.size()<1024 for db " << std::to_string(sourceFile.size()) << ", abort" << std::endl;
                    abort();
                }
                const QByteArray &data=sourceFile.readAll();
                if(data.size()<1024)
                {
                    std::cerr << "data.size()<1024 for db " << std::to_string(data.size()) << ", abort" << std::endl;
                    abort();
                }
                if(destinationFile.open(QIODevice::WriteOnly))
                {
                    if(!destinationFile.write(data))
                        abort();

                    destinationFile.close();
                }
                else
                    abort();

                sourceFile.close();
            }
            else
                abort();
            if(!destinationFile.setPermissions(destinationFile.permissions() | QFileDevice::WriteOwner | QFileDevice::WriteUser))
                abort();
        }
    }
/*    if(destinationFile.size()<1024)
    {
        std::cerr << "destination.size()<1024 for db " << std::to_string(destinationFile.size()) << ", abort" << std::endl;
        abort();
    }
    else
        std::cout << "database config size is: " << std::to_string(destinationFile.size())  << std::endl;*/

    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(destination);

    if (!database.open())
    {
        std::cerr << "Error: connection with database fail" << std::endl;
        abort();
    }
}
