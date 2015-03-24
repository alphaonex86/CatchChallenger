#include "CharactersGroupForLogin.h"
#include <iostream>
#include <QDebug>

using namespace CatchChallenger;

int CharactersGroupForLogin::serverWaitedToBeReady=0;
QHash<QString,CharactersGroupForLogin *> CharactersGroupForLogin::databaseBaseCommonList;

CharactersGroupForLogin::CharactersGroupForLogin(const char * const db,const char * const host,const char * const login,const char * const pass,const quint8 &considerDownAfterNumberOfTry,const quint8 &tryInterval) :
    databaseBaseCommon(new EpollPostgresql())
{
    databaseBaseCommon->considerDownAfterNumberOfTry=considerDownAfterNumberOfTry;
    databaseBaseCommon->tryInterval=tryInterval;
    if(!databaseBaseCommon->syncConnect(host,db,login,pass))
    {
        std::cerr << "Connect to login database failed:" << databaseBaseCommon->errorMessage() << std::endl;
        abort();
    }

    load_clan_max_id();
}

CharactersGroupForLogin::~CharactersGroupForLogin()
{
    if(databaseBaseCommon!=NULL)
    {
        delete databaseBaseCommon;
        databaseBaseCommon=NULL;
    }
}

void CharactersGroupForLogin::load_clan_max_id()
{
    maxClanId=0;
    QString queryText;
    switch(databaseBaseCommon->databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QLatin1String("SELECT `id` FROM `clan` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QLatin1String("SELECT id FROM clan ORDER BY id DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QLatin1String("SELECT id FROM clan ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroupForLogin::load_clan_max_id_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        abort();
    }
}

void CharactersGroupForLogin::load_clan_max_id_static(void *object)
{
    static_cast<CharactersGroupForLogin *>(object)->load_clan_max_id_return();
}

void CharactersGroupForLogin::load_clan_max_id_return()
{
    maxClanId=0;
    while(databaseBaseCommon->next())
    {
        bool ok;
        //not +1 because incremented before use
        maxClanId=QString(databaseBaseCommon->value(0)).toUInt(&ok);
        if(!ok)
        {
            std::cerr << "Max clan id is failed to convert to number" << std::endl;
            abort();
        }
    }
    load_character_max_id();
}

void CharactersGroupForLogin::load_character_max_id()
{
    QString queryText;
    switch(databaseBaseCommon->databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QLatin1String("SELECT `id` FROM `character` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QLatin1String("SELECT id FROM character ORDER BY id DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QLatin1String("SELECT id FROM character ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroupForLogin::load_character_max_id_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        abort();
    }
    maxCharacterId=0;
}

void CharactersGroupForLogin::load_character_max_id_static(void *object)
{
    static_cast<CharactersGroupForLogin *>(object)->load_character_max_id_return();
}

void CharactersGroupForLogin::load_character_max_id_return()
{
    while(databaseBaseCommon->next())
    {
        bool ok;
        //not +1 because incremented before use
        maxCharacterId=QString(databaseBaseCommon->value(0)).toUInt(&ok);
        if(!ok)
        {
            std::cerr << "Max character id is failed to convert to number" << std::endl;
            abort();
        }
    }
    load_monsters_max_id();
}

void CharactersGroupForLogin::load_monsters_max_id()
{
    maxMonsterId=1;
    QString queryText;
    switch(databaseBaseCommon->databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QLatin1String("SELECT `id` FROM `monster` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QLatin1String("SELECT id FROM monster ORDER BY id DESC LIMIT 0,1;");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QLatin1String("SELECT id FROM monster ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroupForLogin::load_monsters_max_id_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        abort();//stop because can't do the first db access
    }
    return;
}

void CharactersGroupForLogin::load_monsters_max_id_static(void *object)
{
    static_cast<CharactersGroupForLogin *>(object)->load_monsters_max_id_return();
}

void CharactersGroupForLogin::load_monsters_max_id_return()
{
    while(databaseBaseCommon->next())
    {
        bool ok;
        //not +1 because incremented before use
        maxMonsterId=QString(databaseBaseCommon->value(0)).toUInt(&ok);
        if(!ok)
        {
            std::cerr << "Max monster id is failed to convert to number" << std::endl;
            abort();
        }
    }

    databaseBaseCommon->syncDisconnect();
    databaseBaseCommon=NULL;
    CharactersGroupForLogin::serverWaitedToBeReady--;
}

BaseClassSwitch::Type CharactersGroupForLogin::getType() const
{
    return BaseClassSwitch::Type::Client;
}
