#include "BaseServerCommon.h"

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
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServerCommon::preload_dictionary_allow_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        criticalDatabaseQueryFailed();return;//stop because can't resolv the name
    }
}

void BaseServerCommon::preload_dictionary_allow_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_dictionary_allow_return();
}

void BaseServerCommon::preload_dictionary_allow_return()
{
    GlobalServerData::serverPrivateVariables.dictionary_allow_reverse << 0x00 << 0x00;
    bool haveAllowClan=false;
    QString allowClan(QStringLiteral("clan"));
    int lastId=0;
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        bool ok;
        lastId=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        const QString &allow=QString(GlobalServerData::serverPrivateVariables.db.value(1));
        if(GlobalServerData::serverPrivateVariables.dictionary_allow.size()<=lastId)
        {
            int index=GlobalServerData::serverPrivateVariables.dictionary_allow.size();
            while(index<=lastId)
            {
                GlobalServerData::serverPrivateVariables.dictionary_allow << ActionAllow_Nothing;
                index++;
            }
        }
        if(allow==allowClan)
        {
            haveAllowClan=true;
            GlobalServerData::serverPrivateVariables.dictionary_allow[lastId]=ActionAllow_Clan;
            GlobalServerData::serverPrivateVariables.dictionary_allow_reverse[ActionAllow_Clan]=lastId;
        }
        else
            GlobalServerData::serverPrivateVariables.dictionary_allow << ActionAllow_Nothing;
    }
    GlobalServerData::serverPrivateVariables.db.clear();
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
        if(!GlobalServerData::serverPrivateVariables.db.asyncWrite(queryText.toLatin1()))
        {
            qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
            criticalDatabaseQueryFailed();return;//stop because can't resolv the name
        }
        while(GlobalServerData::serverPrivateVariables.dictionary_allow.size()<lastId)
            GlobalServerData::serverPrivateVariables.dictionary_allow << ActionAllow_Nothing;
        GlobalServerData::serverPrivateVariables.dictionary_allow << ActionAllow_Clan;
        GlobalServerData::serverPrivateVariables.dictionary_allow_reverse[ActionAllow_Clan]=lastId;
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
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServerCommon::preload_dictionary_reputation_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
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
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        bool ok;
        lastId=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        const QString &reputation=QString(GlobalServerData::serverPrivateVariables.db.value(1));
        if(GlobalServerData::serverPrivateVariables.dictionary_reputation.size()<=lastId)
        {
            int index=GlobalServerData::serverPrivateVariables.dictionary_reputation.size();
            while(index<=lastId)
            {
                GlobalServerData::serverPrivateVariables.dictionary_reputation << -1;
                index++;
            }
        }
        if(reputationResolution.contains(reputation))
        {
            GlobalServerData::serverPrivateVariables.dictionary_reputation[lastId]=reputationResolution.value(reputation);
            foundReputation << reputation;
            CommonDatapack::commonDatapack.reputation[reputationResolution.value(reputation)].reverse_database_id=lastId;
        }
    }
    GlobalServerData::serverPrivateVariables.db.clear();
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
            if(!GlobalServerData::serverPrivateVariables.db.asyncWrite(queryText.toLatin1()))
            {
                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
                criticalDatabaseQueryFailed();return;//stop because can't resolv the name
            }
            while(GlobalServerData::serverPrivateVariables.dictionary_reputation.size()<=lastId)
                GlobalServerData::serverPrivateVariables.dictionary_reputation << -1;
            GlobalServerData::serverPrivateVariables.dictionary_reputation[lastId]=index;
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
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServerCommon::preload_dictionary_skin_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
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
        while(index<GlobalServerData::serverPrivateVariables.skinList.size())
        {
            GlobalServerData::serverPrivateVariables.dictionary_skin_reverse << 0;
            index++;
        }
    }
    QSet<QString> foundSkin;
    int lastId=0;
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        bool ok;
        lastId=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        const QString &skin=QString(GlobalServerData::serverPrivateVariables.db.value(1));
        if(GlobalServerData::serverPrivateVariables.dictionary_skin.size()<=lastId)
        {
            int index=GlobalServerData::serverPrivateVariables.dictionary_skin.size();
            while(index<=lastId)
            {
                GlobalServerData::serverPrivateVariables.dictionary_skin << 0;
                index++;
            }
        }
        if(GlobalServerData::serverPrivateVariables.skinList.contains(skin))
        {
            const quint8 &internalValue=GlobalServerData::serverPrivateVariables.skinList.value(skin);
            GlobalServerData::serverPrivateVariables.dictionary_skin[lastId]=internalValue;
            GlobalServerData::serverPrivateVariables.dictionary_skin_reverse[internalValue]=lastId;
            foundSkin << skin;
        }
    }
    GlobalServerData::serverPrivateVariables.db.clear();
    QHashIterator<QString,quint8> i(GlobalServerData::serverPrivateVariables.skinList);
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
            if(!GlobalServerData::serverPrivateVariables.db.asyncWrite(queryText.toLatin1()))
            {
                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
                criticalDatabaseQueryFailed();return;//stop because can't resolv the name
            }
            while(GlobalServerData::serverPrivateVariables.dictionary_skin.size()<lastId)
                GlobalServerData::serverPrivateVariables.dictionary_skin << 0;
            GlobalServerData::serverPrivateVariables.dictionary_skin << i.value();
            GlobalServerData::serverPrivateVariables.dictionary_skin_reverse[i.value()]=lastId;
        }
    }
    SQL_common_load_finish();
}
