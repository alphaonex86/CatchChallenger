#include "CharactersGroup.h"
#include "EpollServerLoginMaster.h"
#include <iostream>
#include <QDebug>

using namespace CatchChallenger;

int CharactersGroup::serverWaitedToBeReady=0;
QHash<QString,CharactersGroup *> CharactersGroup::hash;
QList<CharactersGroup *> CharactersGroup::list;

CharactersGroup::CharactersGroup(const char * const db,const char * const host,const char * const login,const char * const pass,const quint8 &considerDownAfterNumberOfTry,const quint8 &tryInterval,const QString &name) :
    databaseBaseCommon(new EpollPostgresql())
{
    this->name=name;
    databaseBaseCommon->considerDownAfterNumberOfTry=considerDownAfterNumberOfTry;
    databaseBaseCommon->tryInterval=tryInterval;
    if(!databaseBaseCommon->syncConnect(host,db,login,pass))
    {
        std::cerr << "Connect to login database failed:" << databaseBaseCommon->errorMessage() << std::endl;
        abort();
    }

    load_clan_max_id();
}

CharactersGroup::~CharactersGroup()
{
    if(databaseBaseCommon!=NULL)
    {
        delete databaseBaseCommon;
        databaseBaseCommon=NULL;
    }
}

void CharactersGroup::load_clan_max_id()
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
    if(databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroup::load_clan_max_id_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        abort();
    }
}

void CharactersGroup::load_clan_max_id_static(void *object)
{
    static_cast<CharactersGroup *>(object)->load_clan_max_id_return();
}

void CharactersGroup::load_clan_max_id_return()
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

void CharactersGroup::load_character_max_id()
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
    if(databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroup::load_character_max_id_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        abort();
    }
    maxCharacterId=0;
}

void CharactersGroup::load_character_max_id_static(void *object)
{
    static_cast<CharactersGroup *>(object)->load_character_max_id_return();
}

void CharactersGroup::load_character_max_id_return()
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

void CharactersGroup::load_monsters_max_id()
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
    if(databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroup::load_monsters_max_id_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
        abort();//stop because can't do the first db access
    }
}

void CharactersGroup::load_monsters_max_id_static(void *object)
{
    static_cast<CharactersGroup *>(object)->load_monsters_max_id_return();
}

void CharactersGroup::load_monsters_max_id_return()
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

    /*to register new connected game server
    databaseBaseCommon->syncDisconnect();
    databaseBaseCommon=NULL;*/
    CharactersGroup::serverWaitedToBeReady--;
}

BaseClassSwitch::Type CharactersGroup::getType() const
{
    return BaseClassSwitch::Type::Client;
}

CharactersGroup::InternalGameServer * CharactersGroup::addGameServerUniqueKey(void * const link, const quint32 &uniqueKey, const QString &host,
                                                                              const quint16 &port, const QString &metaData, const QString &logicalGroup,
                                                                              const quint16 &currentPlayer, const quint16 &maxPlayer)
{
    InternalGameServer tempServer;
    tempServer.host=host;//QString::fromUtf8(hostData,hostDataSize)
    tempServer.port=port;
    tempServer.link=link;

    tempServer.logicalGroup=logicalGroup;
    tempServer.metaData=metaData;
    tempServer.uniqueKey=uniqueKey;

    tempServer.currentPlayer=currentPlayer;
    tempServer.maxPlayer=maxPlayer;

    gameServers[uniqueKey]=tempServer;
    gameServersLinkToUniqueKey[link]=uniqueKey;

    broadcastGameServerChange();

    return &gameServers[uniqueKey];
}

void CharactersGroup::removeGameServerUniqueKey(void * const link)
{
    gameServers.remove(gameServersLinkToUniqueKey.value(link));
    gameServersLinkToUniqueKey.remove(link);

    broadcastGameServerChange();
}

void CharactersGroup::broadcastGameServerChange()
{
    EpollServerLoginMaster::epollServerLoginMaster->doTheServerList();
    EpollServerLoginMaster::epollServerLoginMaster->doTheReplyCache();

    EpollClientLoginMaster::broadcastGameServerChange();
}

bool CharactersGroup::containsGameServerUniqueKey(const quint32 &serverUniqueKey) const
{
    return gameServers.contains(serverUniqueKey);
}
