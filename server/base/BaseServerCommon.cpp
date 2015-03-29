#include "BaseServerCommon.h"
#include "../../general/base/CommonDatapack.h"

using namespace CatchChallenger;

void BaseServerCommon::preload_dictionary_allow()
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case GameServerSettings::Database::DatabaseType_Mysql:
            queryText=QStringLiteral("SELECT `id`,`allow` FROM `dictionary_allow` ORDER BY `allow`");
        break;
        case GameServerSettings::Database::DatabaseType_SQLite:
            queryText=QStringLiteral("SELECT id,allow FROM dictionary_allow ORDER BY allow");
        break;
        case GameServerSettings::Database::DatabaseType_PostgreSQL:
            queryText=QStringLiteral("SELECT id,allow FROM dictionary_allow ORDER BY allow");
        break;
    }
    if(db.asyncRead(queryText.toLatin1(),this,&BaseServerCommon::preload_dictionary_allow_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(db.errorMessage());
        criticalDatabaseQueryFailed();return;//stop because can't resolv the name
    }
}

void BaseServerCommon::preload_dictionary_allow_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_dictionary_allow_return();
}

void BaseServerCommon::preload_dictionary_allow_return()
{
    dictionary_allow_internal_to_database << 0x00 << 0x00;
    bool haveAllowClan=false;
    QString allowClan(QStringLiteral("clan"));
    int lastId=0;
    while(db.next())
    {
        bool ok;
        lastId=QString(db.value(0)).toUInt(&ok);
        const QString &allow=QString(db.value(1));
        if(dictionary_allow_database_to_internal.size()<=lastId)
        {
            int index=dictionary_allow_database_to_internal.size();
            while(index<=lastId)
            {
                dictionary_allow_database_to_internal << ActionAllow_Nothing;
                index++;
            }
        }
        if(allow==allowClan)
        {
            haveAllowClan=true;
            dictionary_allow_database_to_internal[lastId]=ActionAllow_Clan;
            dictionary_allow_internal_to_database[ActionAllow_Clan]=lastId;
        }
        else
            dictionary_allow_database_to_internal << ActionAllow_Nothing;
    }
    db.clear();
    if(!haveAllowClan)
    {
        lastId++;
        QString queryText;
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case GameServerSettings::Database::DatabaseType_Mysql:
                queryText=QStringLiteral("INSERT INTO `dictionary_allow`(`id`,`allow`) VALUES(%1,'clan');").arg(lastId);
            break;
            case GameServerSettings::Database::DatabaseType_SQLite:
                queryText=QStringLiteral("INSERT INTO dictionary_allow(id,allow) VALUES(%1,'clan');").arg(lastId);
            break;
            case GameServerSettings::Database::DatabaseType_PostgreSQL:
                queryText=QStringLiteral("INSERT INTO dictionary_allow(id,allow) VALUES(%1,'clan');").arg(lastId);
            break;
        }
        if(!db.asyncWrite(queryText.toLatin1()))
        {
            qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(db.errorMessage());
            criticalDatabaseQueryFailed();return;//stop because can't resolv the name
        }
        while(dictionary_allow_database_to_internal.size()<lastId)
            dictionary_allow_database_to_internal << ActionAllow_Nothing;
        dictionary_allow_database_to_internal << ActionAllow_Clan;
        dictionary_allow_internal_to_database[ActionAllow_Clan]=lastId;
    }
    preload_dictionary_reputation();
}

void BaseServerCommon::preload_dictionary_reputation()
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case GameServerSettings::Database::DatabaseType_Mysql:
            queryText=QStringLiteral("SELECT `id`,`reputation` FROM `dictionary_reputation` ORDER BY `reputation`");
        break;
        case GameServerSettings::Database::DatabaseType_SQLite:
            queryText=QStringLiteral("SELECT id,reputation FROM dictionary_reputation ORDER BY reputation");
        break;
        case GameServerSettings::Database::DatabaseType_PostgreSQL:
            queryText=QStringLiteral("SELECT id,reputation FROM dictionary_reputation ORDER BY reputation");
        break;
    }
    if(db.asyncRead(queryText.toLatin1(),this,&BaseServerCommon::preload_dictionary_reputation_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(db.errorMessage());
        criticalDatabaseQueryFailed();return;//stop because can't resolv the name
    }
}

void BaseServerCommon::preload_dictionary_reputation_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_dictionary_reputation_return();
}

void BaseServerCommon::preload_dictionary_reputation_return()
{
    QHash<QString,quint8> reputationResolution;
    {
        quint8 index=0;
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            reputationResolution[CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    QSet<QString> foundReputation;
    int lastId=0;
    while(db.next())
    {
        bool ok;
        lastId=QString(db.value(0)).toUInt(&ok);
        const QString &reputation=QString(db.value(1));
        if(dictionary_reputation_database_to_internal.size()<=lastId)
        {
            int index=dictionary_reputation_database_to_internal.size();
            while(index<=lastId)
            {
                dictionary_reputation_database_to_internal << -1;
                index++;
            }
        }
        if(reputationResolution.contains(reputation))
        {
            dictionary_reputation_database_to_internal[lastId]=reputationResolution.value(reputation);
            foundReputation << reputation;
            CommonDatapack::commonDatapack.reputation[reputationResolution.value(reputation)].reverse_database_id=lastId;
        }
    }
    db.clear();
    int index=0;
    while(index<CommonDatapack::commonDatapack.reputation.size())
    {
        const QString &reputation=CommonDatapack::commonDatapack.reputation.at(index).name;
        if(!foundReputation.contains(reputation))
        {
            lastId++;
            QString queryText;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case GameServerSettings::Database::DatabaseType_Mysql:
                    queryText=QStringLiteral("INSERT INTO `dictionary_reputation`(`id`,`reputation`) VALUES(%1,'%2');").arg(lastId).arg(reputation);
                break;
                case GameServerSettings::Database::DatabaseType_SQLite:
                    queryText=QStringLiteral("INSERT INTO dictionary_reputation(id,reputation) VALUES(%1,'%2');").arg(lastId).arg(reputation);
                break;
                case GameServerSettings::Database::DatabaseType_PostgreSQL:
                    queryText=QStringLiteral("INSERT INTO dictionary_reputation(id,reputation) VALUES(%1,'%2');").arg(lastId).arg(reputation);
                break;
            }
            if(!db.asyncWrite(queryText.toLatin1()))
            {
                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(db.errorMessage());
                criticalDatabaseQueryFailed();return;//stop because can't resolv the name
            }
            while(dictionary_reputation_database_to_internal.size()<=lastId)
                dictionary_reputation_database_to_internal << -1;
            dictionary_reputation_database_to_internal[lastId]=index;
            CommonDatapack::commonDatapack.reputation[index].reverse_database_id=lastId;
        }
        index++;
    }
    preload_dictionary_skin();
}

void BaseServerCommon::preload_dictionary_skin()
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case GameServerSettings::Database::DatabaseType_Mysql:
            queryText=QStringLiteral("SELECT `id`,`skin` FROM `dictionary_skin` ORDER BY `skin`");
        break;
        case GameServerSettings::Database::DatabaseType_SQLite:
            queryText=QStringLiteral("SELECT id,skin FROM dictionary_skin ORDER BY skin");
        break;
        case GameServerSettings::Database::DatabaseType_PostgreSQL:
            queryText=QStringLiteral("SELECT id,skin FROM dictionary_skin ORDER BY skin");
        break;
    }
    if(db.asyncRead(queryText.toLatin1(),this,&BaseServerCommon::preload_dictionary_skin_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(db.errorMessage());
        criticalDatabaseQueryFailed();return;//stop because can't resolv the name
    }
}

void BaseServerCommon::preload_dictionary_skin_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_dictionary_skin_return();
}

void BaseServerCommon::preload_dictionary_skin_return()
{
    {
        int index=0;
        while(index<skinList.size())
        {
            dictionary_skin_internal_to_database << 0;
            index++;
        }
    }
    QSet<QString> foundSkin;
    int lastId=0;
    while(db.next())
    {
        bool ok;
        lastId=QString(db.value(0)).toUInt(&ok);
        const QString &skin=QString(db.value(1));
        if(dictionary_skin_database_to_internal.size()<=lastId)
        {
            int index=dictionary_skin_database_to_internal.size();
            while(index<=lastId)
            {
                dictionary_skin_database_to_internal << 0;
                index++;
            }
        }
        if(skinList.contains(skin))
        {
            const quint8 &internalValue=skinList.value(skin);
            dictionary_skin_database_to_internal[lastId]=internalValue;
            dictionary_skin_internal_to_database[internalValue]=lastId;
            foundSkin << skin;
        }
    }
    db.clear();
    QHashIterator<QString,quint8> i(skinList);
    while (i.hasNext()) {
        i.next();
        const QString &skin=i.key();
        if(!foundSkin.contains(skin))
        {
            lastId++;
            QString queryText;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case GameServerSettings::Database::DatabaseType_Mysql:
                    queryText=QStringLiteral("INSERT INTO `dictionary_skin`(`id`,`skin`) VALUES(%1,'%2');").arg(lastId).arg(skin);
                break;
                case GameServerSettings::Database::DatabaseType_SQLite:
                    queryText=QStringLiteral("INSERT INTO dictionary_skin(id,skin) VALUES(%1,'%2');").arg(lastId).arg(skin);
                break;
                case GameServerSettings::Database::DatabaseType_PostgreSQL:
                    queryText=QStringLiteral("INSERT INTO dictionary_skin(id,skin) VALUES(%1,'%2');").arg(lastId).arg(skin);
                break;
            }
            if(!db.asyncWrite(queryText.toLatin1()))
            {
                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(db.errorMessage());
                criticalDatabaseQueryFailed();return;//stop because can't resolv the name
            }
            while(dictionary_skin_database_to_internal.size()<lastId)
                dictionary_skin_database_to_internal << 0;
            dictionary_skin_database_to_internal << i.value();
            dictionary_skin_internal_to_database[i.value()]=lastId;
        }
    }
    preload_dictionary_starter();
}

void BaseServerCommon::preload_dictionary_starter()
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case GameServerSettings::Database::DatabaseType_Mysql:
            queryText=QStringLiteral("SELECT `id`,`starter` FROM `dictionary_starter` ORDER BY `starter`");
        break;
        case GameServerSettings::Database::DatabaseType_SQLite:
            queryText=QStringLiteral("SELECT id,starter FROM dictionary_starter ORDER BY starter");
        break;
        case GameServerSettings::Database::DatabaseType_PostgreSQL:
            queryText=QStringLiteral("SELECT id,starter FROM dictionary_starter ORDER BY starter");
        break;
    }
    if(db.asyncRead(queryText.toLatin1(),this,&BaseServerCommon::preload_dictionary_starter_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(db.errorMessage());
        criticalDatabaseQueryFailed();return;//stop because can't resolv the name
    }
}

void BaseServerCommon::preload_dictionary_starter_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_dictionary_starter_return();
}

void BaseServerCommon::preload_dictionary_starter_return()
{
    {
        int index=0;
        while(index<starterList.size())
        {
            dictionary_starter_reverse << 0;
            index++;
        }
    }
    QSet<QString> foundstarter;
    int lastId=0;
    while(db.next())
    {
        bool ok;
        lastId=QString(db.value(0)).toUInt(&ok);
        const QString &starter=QString(db.value(1));
        if(dictionary_starter.size()<=lastId)
        {
            int index=dictionary_starter.size();
            while(index<=lastId)
            {
                dictionary_starter << 0;
                index++;
            }
        }
        if(starterList.contains(starter))
        {
            const quint8 &internalValue=starterList.value(starter);
            dictionary_starter[lastId]=internalValue;
            dictionary_starter_reverse[internalValue]=lastId;
            foundstarter << starter;
        }
    }
    db.clear();
    QHashIterator<QString,quint8> i(starterList);
    while (i.hasNext()) {
        i.next();
        const QString &starter=i.key();
        if(!foundstarter.contains(starter))
        {
            lastId++;
            QString queryText;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case GameServerSettings::Database::DatabaseType_Mysql:
                    queryText=QStringLiteral("INSERT INTO `dictionary_starter`(`id`,`starter`) VALUES(%1,'%2');").arg(lastId).arg(starter);
                break;
                case GameServerSettings::Database::DatabaseType_SQLite:
                    queryText=QStringLiteral("INSERT INTO dictionary_starter(id,starter) VALUES(%1,'%2');").arg(lastId).arg(starter);
                break;
                case GameServerSettings::Database::DatabaseType_PostgreSQL:
                    queryText=QStringLiteral("INSERT INTO dictionary_starter(id,starter) VALUES(%1,'%2');").arg(lastId).arg(starter);
                break;
            }
            if(!db.asyncWrite(queryText.toLatin1()))
            {
                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(db.errorMessage());
                criticalDatabaseQueryFailed();return;//stop because can't resolv the name
            }
            while(dictionary_starter.size()<lastId)
                dictionary_starter << 0;
            dictionary_starter << i.value();
            dictionary_starter_reverse[i.value()]=lastId;
        }
    }
    SQL_common_load_finish();
}

void BaseServerCommon::unload()
{
    dictionary_starter.clear();
    dictionary_starter_reverse.clear();
    dictionary_skin_database_to_internal.clear();
    dictionary_skin_internal_to_database.clear();
    dictionary_reputation_database_to_internal.clear();
    dictionary_allow_database_to_internal.clear();
    dictionary_allow_internal_to_database.clear();
}
