#include "DatabaseBot.h"

#include <QString>
#include <QCoreApplication>
#include <QFile>

#include <iostream>
#include "../libbot/BotAbort.h"

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
            BOT_ABORT();
        else
        {
            if(sourceFile.open(QIODevice::ReadOnly))
            {
                if(sourceFile.size()<1024)
                {
                    std::cerr << "destination.size()<1024 for db " << std::to_string(sourceFile.size()) << ", abort" << std::endl;
                    BOT_ABORT();
                }
                const QByteArray &data=sourceFile.readAll();
                if(data.size()<1024)
                {
                    std::cerr << "data.size()<1024 for db " << std::to_string(data.size()) << ", abort" << std::endl;
                    BOT_ABORT();
                }
                if(destinationFile.open(QIODevice::WriteOnly))
                {
                    if(!destinationFile.write(data))
                        BOT_ABORT();

                    destinationFile.close();
                }
                else
                    BOT_ABORT();

                sourceFile.close();
            }
            else
                BOT_ABORT();
            if(!destinationFile.setPermissions(destinationFile.permissions() | QFileDevice::WriteOwner | QFileDevice::WriteUser))
                BOT_ABORT();
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
        BOT_ABORT();
    }
}
