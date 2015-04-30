#include "BaseServerMasterLoadDictionary.h"
#include "BaseServerMasterSendDatapack.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"

#include <QDebug>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <iostream>

using namespace CatchChallenger;

BaseServerMasterLoadDictionary::BaseServerMasterLoadDictionary() :
    databaseBaseLogin(NULL)
{
}

BaseServerMasterLoadDictionary::~BaseServerMasterLoadDictionary()
{
}

void BaseServerMasterLoadDictionary::load(DatabaseBase * const databaseBase)
{
    this->databaseBaseLogin=databaseBase;
}

void BaseServerMasterLoadDictionary::preload_dictionary_allow()
{
    QString queryText;
    switch(databaseBaseLogin->databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QStringLiteral("SELECT `id`,`allow` FROM `dictionary_allow` ORDER BY `allow`");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QStringLiteral("SELECT id,allow FROM dictionary_allow ORDER BY allow");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QStringLiteral("SELECT id,allow FROM dictionary_allow ORDER BY allow");
        break;
    }
    if(databaseBaseLogin->asyncRead(queryText.toLatin1(),this,&BaseServerMasterLoadDictionary::preload_dictionary_allow_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin->errorMessage());
        abort();//stop because can't resolv the name
    }
}

void BaseServerMasterLoadDictionary::preload_dictionary_allow_static(void *object)
{
    static_cast<BaseServerMasterLoadDictionary *>(object)->preload_dictionary_allow_return();
}

void BaseServerMasterLoadDictionary::preload_dictionary_allow_return()
{
    dictionary_allow_internal_to_database << 0x00 << 0x00;
    bool haveAllowClan=false;
    QString allowClan(QStringLiteral("clan"));
    int lastId=0;
    while(databaseBaseLogin->next())
    {
        bool ok;
        lastId=QString(databaseBaseLogin->value(0)).toUInt(&ok);
        const QString &allow=QString(databaseBaseLogin->value(1));
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
    databaseBaseLogin->clear();
    if(!haveAllowClan)
    {
        lastId++;
        QString queryText;
        switch(databaseBaseLogin->databaseType())
        {
            default:
            case DatabaseBase::Type::Mysql:
                queryText=QStringLiteral("INSERT INTO `dictionary_allow`(`id`,`allow`) VALUES(%1,'clan');").arg(lastId);
            break;
            case DatabaseBase::Type::SQLite:
                queryText=QStringLiteral("INSERT INTO dictionary_allow(id,allow) VALUES(%1,'clan');").arg(lastId);
            break;
            case DatabaseBase::Type::PostgreSQL:
                queryText=QStringLiteral("INSERT INTO dictionary_allow(id,allow) VALUES(%1,'clan');").arg(lastId);
            break;
        }
        if(!databaseBaseLogin->asyncWrite(queryText.toLatin1()))
        {
            qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin->errorMessage());
            abort();//stop because can't resolv the name
        }
        while(dictionary_allow_database_to_internal.size()<lastId)
            dictionary_allow_database_to_internal << ActionAllow_Nothing;
        dictionary_allow_database_to_internal << ActionAllow_Clan;
        dictionary_allow_internal_to_database[ActionAllow_Clan]=lastId;
    }
    preload_dictionary_reputation();
}

void BaseServerMasterLoadDictionary::preload_dictionary_reputation()
{
    QString queryText;
    switch(databaseBaseLogin->databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QStringLiteral("SELECT `id`,`reputation` FROM `dictionary_reputation` ORDER BY `reputation`");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QStringLiteral("SELECT id,reputation FROM dictionary_reputation ORDER BY reputation");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QStringLiteral("SELECT id,reputation FROM dictionary_reputation ORDER BY reputation");
        break;
    }
    if(databaseBaseLogin->asyncRead(queryText.toLatin1(),this,&BaseServerMasterLoadDictionary::preload_dictionary_reputation_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin->errorMessage());
        abort();//stop because can't resolv the name
    }
}

void BaseServerMasterLoadDictionary::preload_dictionary_reputation_static(void *object)
{
    static_cast<BaseServerMasterLoadDictionary *>(object)->preload_dictionary_reputation_return();
}

void BaseServerMasterLoadDictionary::preload_dictionary_reputation_return()
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
    while(databaseBaseLogin->next())
    {
        bool ok;
        lastId=QString(databaseBaseLogin->value(0)).toUInt(&ok);
        const QString &reputation=QString(databaseBaseLogin->value(1));
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
    databaseBaseLogin->clear();
    int index=0;
    while(index<CommonDatapack::commonDatapack.reputation.size())
    {
        const QString &reputation=CommonDatapack::commonDatapack.reputation.at(index).name;
        if(!foundReputation.contains(reputation))
        {
            lastId++;
            QString queryText;
            switch(databaseBaseLogin->databaseType())
            {
                default:
                case DatabaseBase::Type::Mysql:
                    queryText=QStringLiteral("INSERT INTO `dictionary_reputation`(`id`,`reputation`) VALUES(%1,'%2');").arg(lastId).arg(reputation);
                break;
                case DatabaseBase::Type::SQLite:
                    queryText=QStringLiteral("INSERT INTO dictionary_reputation(id,reputation) VALUES(%1,'%2');").arg(lastId).arg(reputation);
                break;
                case DatabaseBase::Type::PostgreSQL:
                    queryText=QStringLiteral("INSERT INTO dictionary_reputation(id,reputation) VALUES(%1,'%2');").arg(lastId).arg(reputation);
                break;
            }
            if(!databaseBaseLogin->asyncWrite(queryText.toLatin1()))
            {
                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin->errorMessage());
                abort();//stop because can't resolv the name
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

void BaseServerMasterLoadDictionary::preload_dictionary_skin()
{
    QString queryText;
    switch(databaseBaseLogin->databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QStringLiteral("SELECT `id`,`skin` FROM `dictionary_skin` ORDER BY `skin`");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QStringLiteral("SELECT id,skin FROM dictionary_skin ORDER BY skin");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QStringLiteral("SELECT id,skin FROM dictionary_skin ORDER BY skin");
        break;
    }
    if(databaseBaseLogin->asyncRead(queryText.toLatin1(),this,&BaseServerMasterLoadDictionary::preload_dictionary_skin_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin->errorMessage());
        abort();//stop because can't resolv the name
    }
}

void BaseServerMasterLoadDictionary::preload_dictionary_skin_static(void *object)
{
    static_cast<BaseServerMasterLoadDictionary *>(object)->preload_dictionary_skin_return();
}

void BaseServerMasterLoadDictionary::preload_dictionary_skin_return()
{
    {
        int index=0;
        while(index<BaseServerMasterSendDatapack::skinList.size())
        {
            dictionary_skin_internal_to_database << 0;
            index++;
        }
    }
    QSet<QString> foundSkin;
    int lastId=0;
    while(databaseBaseLogin->next())
    {
        bool ok;
        lastId=QString(databaseBaseLogin->value(0)).toUInt(&ok);
        const QString &skin=QString(databaseBaseLogin->value(1));
        if(dictionary_skin_database_to_internal.size()<=lastId)
        {
            int index=dictionary_skin_database_to_internal.size();
            while(index<=lastId)
            {
                dictionary_skin_database_to_internal << 0;
                index++;
            }
        }
        if(BaseServerMasterSendDatapack::skinList.contains(skin))
        {
            const quint8 &internalValue=BaseServerMasterSendDatapack::skinList.value(skin);
            dictionary_skin_database_to_internal[lastId]=internalValue;
            dictionary_skin_internal_to_database[internalValue]=lastId;
            foundSkin << skin;
        }
    }
    databaseBaseLogin->clear();
    QHashIterator<QString,quint8> i(BaseServerMasterSendDatapack::skinList);
    while (i.hasNext()) {
        i.next();
        const QString &skin=i.key();
        if(!foundSkin.contains(skin))
        {
            lastId++;
            QString queryText;
            switch(databaseBaseLogin->databaseType())
            {
                default:
                case DatabaseBase::Type::Mysql:
                    queryText=QStringLiteral("INSERT INTO `dictionary_skin`(`id`,`skin`) VALUES(%1,'%2');").arg(lastId).arg(skin);
                break;
                case DatabaseBase::Type::SQLite:
                    queryText=QStringLiteral("INSERT INTO dictionary_skin(id,skin) VALUES(%1,'%2');").arg(lastId).arg(skin);
                break;
                case DatabaseBase::Type::PostgreSQL:
                    queryText=QStringLiteral("INSERT INTO dictionary_skin(id,skin) VALUES(%1,'%2');").arg(lastId).arg(skin);
                break;
            }
            if(!databaseBaseLogin->asyncWrite(queryText.toLatin1()))
            {
                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin->errorMessage());
                abort();//stop because can't resolv the name
            }
            while(dictionary_skin_database_to_internal.size()<lastId)
                dictionary_skin_database_to_internal << 0;
            dictionary_skin_database_to_internal << i.value();
            dictionary_skin_internal_to_database[i.value()]=lastId;
        }
    }
    preload_dictionary_starter();
}

void BaseServerMasterLoadDictionary::preload_dictionary_starter()
{
    QString queryText;
    switch(databaseBaseLogin->databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QStringLiteral("SELECT `id`,`starter` FROM `dictionary_starter` ORDER BY `starter`");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QStringLiteral("SELECT id,starter FROM dictionary_starter ORDER BY starter");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QStringLiteral("SELECT id,starter FROM dictionary_starter ORDER BY starter");
        break;
    }
    if(databaseBaseLogin->asyncRead(queryText.toLatin1(),this,&BaseServerMasterLoadDictionary::preload_dictionary_starter_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin->errorMessage());
        abort();//stop because can't resolv the name
    }
}

void BaseServerMasterLoadDictionary::preload_dictionary_starter_static(void *object)
{
    static_cast<BaseServerMasterLoadDictionary *>(object)->preload_dictionary_starter_return();
}

void BaseServerMasterLoadDictionary::preload_dictionary_starter_return()
{
    QHash<QString,quint8> profileNameToId;
    {
        int index=0;
        while(index<CommonDatapack::commonDatapack.profileList.size())
        {
            const Profile &profile=CommonDatapack::commonDatapack.profileList.at(index);
            profileNameToId[profile.id]=index;
            index++;
        }
    }
    {
        int index=0;
        while(index<CommonDatapack::commonDatapack.profileList.size())
        {
            dictionary_starter_internal_to_database << 0;
            index++;
        }
    }
    QSet<QString> foundstarter;
    int lastId=0;
    while(databaseBaseLogin->next())
    {
        bool ok;
        lastId=QString(databaseBaseLogin->value(0)).toUInt(&ok);
        const QString &starter=QString(databaseBaseLogin->value(1));
        if(dictionary_starter_database_to_internal.size()<=lastId)
        {
            int index=dictionary_starter_database_to_internal.size();
            while(index<=lastId)
            {
                dictionary_starter_database_to_internal << 0;
                index++;
            }
        }
        if(profileNameToId.contains(starter))
        {
            const quint8 &internalValue=profileNameToId.value(starter);
            dictionary_starter_database_to_internal[lastId]=internalValue;
            dictionary_starter_internal_to_database[internalValue]=lastId;
            foundstarter << CommonDatapack::commonDatapack.profileList.at(internalValue).id;
        }
    }
    databaseBaseLogin->clear();
    int index=0;
    while(index<CommonDatapack::commonDatapack.profileList.size())
    {
        const Profile &profile=CommonDatapack::commonDatapack.profileList.at(index);
        if(!foundstarter.contains(profile.id))
        {
            lastId++;
            QString queryText;
            switch(databaseBaseLogin->databaseType())
            {
                default:
                case DatabaseBase::Type::Mysql:
                    queryText=QStringLiteral("INSERT INTO `dictionary_starter`(`id`,`starter`) VALUES(%1,'%2');").arg(lastId).arg(profile.id);
                break;
                case DatabaseBase::Type::SQLite:
                    queryText=QStringLiteral("INSERT INTO dictionary_starter(id,starter) VALUES(%1,'%2');").arg(lastId).arg(profile.id);
                break;
                case DatabaseBase::Type::PostgreSQL:
                    queryText=QStringLiteral("INSERT INTO dictionary_starter(id,starter) VALUES(%1,'%2');").arg(lastId).arg(profile.id);
                break;
            }
            if(!databaseBaseLogin->asyncWrite(queryText.toLatin1()))
            {
                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin->errorMessage());
                abort();//stop because can't resolv the name
            }
            while(dictionary_starter_database_to_internal.size()<lastId)
                dictionary_starter_database_to_internal << 0;
            dictionary_starter_database_to_internal << index;
            dictionary_starter_internal_to_database[index]=lastId;
        }
        index++;
    }
    SQL_common_load_finish();
}

void BaseServerMasterLoadDictionary::unload()
{
    dictionary_starter_database_to_internal.clear();
    dictionary_starter_internal_to_database.clear();
    dictionary_skin_database_to_internal.clear();
    dictionary_skin_internal_to_database.clear();
    dictionary_reputation_database_to_internal.clear();
    dictionary_allow_database_to_internal.clear();
    dictionary_allow_internal_to_database.clear();
    databaseBaseLogin=NULL;
}
