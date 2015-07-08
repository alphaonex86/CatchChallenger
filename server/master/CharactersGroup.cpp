#include "CharactersGroup.h"
#include "EpollServerLoginMaster.h"
#include <iostream>
#include <QDebug>
#include <QDateTime>

using namespace CatchChallenger;

int CharactersGroup::serverWaitedToBeReady=0;
QHash<QString,CharactersGroup *> CharactersGroup::hash;
QList<CharactersGroup *> CharactersGroup::list;

CharactersGroup::CharactersGroup(const char * const db, const char * const host, const char * const login, const char * const pass, const quint8 &considerDownAfterNumberOfTry, const quint8 &tryInterval, const QString &name) :
    databaseBaseCommon(new EpollPostgresql())
{
    this->index=0;
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
                                                                              const quint16 &port, const QString &metaData, const quint32 &logicalGroupIndex,
                                                                              const quint16 &currentPlayer, const quint16 &maxPlayer,const QSet<quint32> &lockedAccount)
{
    //old locked account
    if(lockedAccountByDisconnectedServer.contains(uniqueKey))
    {
        //new key found on master server
        QSet<quint32>::const_iterator i = lockedAccountByDisconnectedServer.value(uniqueKey).constBegin();
        while (i != lockedAccountByDisconnectedServer.value(uniqueKey).constEnd()) {
            if(!lockedAccount.contains(*i))
            {
                //was disconnected from the last game server disconnection
                this->lockedAccount[*i]=QDateTime::currentMSecsSinceEpoch()/1000+5;//wait 5s before reconnect
            }
            ++i;
        }
        lockedAccountByDisconnectedServer.remove(uniqueKey);
    }
    InternalGameServer tempServer;
    tempServer.host=host;//QString::fromUtf8(hostData,hostDataSize)
    tempServer.port=port;
    tempServer.link=link;

    tempServer.logicalGroupIndex=logicalGroupIndex;
    tempServer.metaData=metaData;
    tempServer.uniqueKey=uniqueKey;

    tempServer.currentPlayer=currentPlayer;
    tempServer.maxPlayer=maxPlayer;

    //new key found on game server, mostly when the master server is restarted
    {
        QSet<quint32>::const_iterator i = lockedAccount.constBegin();
        while (i != lockedAccount.constEnd()) {
            if(this->lockedAccount.contains(*i))
            {
                const quint64 &timeLock=this->lockedAccount.value(*i);
                //drop the timeouted lock
                if(timeLock>0 && timeLock<(quint64)QDateTime::currentMSecsSinceEpoch()/1000)
                {
                    tempServer.lockedAccount << *i;
                    this->lockedAccount[*i]=0;
                }
                /// \todo already locked, diconnect all to be more safe, but bug here! network split? block during 5min this character login
                else
                {
                    //find the other game server and disconnect character on it
                    if(Q_UNLIKELY(timeLock==0))
                    {
                        QHashIterator<quint32/*serverUniqueKey*/,InternalGameServer> j(gameServers);
                        while (j.hasNext()) {
                            j.next();
                            if(j.value().lockedAccount.contains(*i))
                            {
                                static_cast<EpollClientLoginMaster *>(j.value().link)->disconnectForDuplicateConnexionDetected(*i);
                                gameServers[j.key()].lockedAccount.remove(*i);
                            }
                        }
                        this->lockedAccount[*i]=QDateTime::currentMSecsSinceEpoch()/1000+5*60;//wait 5min before reconnect
                        static_cast<EpollClientLoginMaster *>(link)->disconnectForDuplicateConnexionDetected(*i);
                    }
                    else
                    {
                        //recent normal disconnected, just wait the 5s timeout
                        static_cast<EpollClientLoginMaster *>(link)->disconnectForDuplicateConnexionDetected(*i);
                    }
                }
            }
            else
            {
                tempServer.lockedAccount << *i;
                this->lockedAccount[*i]=0;
            }
            ++i;
        }
    }

    gameServers[uniqueKey]=tempServer;
    gameServersLinkToUniqueKey[link]=uniqueKey;

    return &gameServers[uniqueKey];
}

void CharactersGroup::removeGameServerUniqueKey(void * const link)
{
    const quint32 &uniqueKey=gameServersLinkToUniqueKey.value(link);
    const InternalGameServer &internalGameServer=gameServers.value(uniqueKey);
    lockedAccountByDisconnectedServer.insert(uniqueKey,internalGameServer.lockedAccount);
    gameServers.remove(uniqueKey);
    gameServersLinkToUniqueKey.remove(link);
}

bool CharactersGroup::containsGameServerUniqueKey(const quint32 &serverUniqueKey) const
{
    return gameServers.contains(serverUniqueKey);
}

bool CharactersGroup::characterIsLocked(const quint32 &characterId)
{
    if(lockedAccount.contains(characterId))
    {
        if(lockedAccount.value(characterId)==0)
            return true;
        if(lockedAccount.value(characterId)<(quint64)QDateTime::currentMSecsSinceEpoch()/1000)
        {
            lockedAccount.remove(characterId);
            return false;
        }
        else
            return true;
    }
    else
        return false;
}

//need check if is already locked before this call
//don't apply on InternalGameServer
void CharactersGroup::lockTheCharacter(const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(lockedAccount.contains(characterId))
    {
        if(lockedAccount.contains(characterId)==0)
        {
            qDebug() << QStringLiteral("lockedAccount already contains: %1").arg(characterId);
            return;
        }
        if(lockedAccount.contains(characterId)>QDateTime::currentMSecsSinceEpoch()/1000)
        {
            qDebug() << QStringLiteral("lockedAccount already contains not finished timeout: %1").arg(characterId);
            return;
        }
    }
    QHashIterator<quint32/*serverUniqueKey*/,InternalGameServer> j(gameServers);
    while (j.hasNext()) {
        j.next();
        if(j.value().lockedAccount.contains(characterId))
        {
            qDebug() << QStringLiteral("lockedAccount already contains on a game server: %1").arg(characterId);
            return;
        }
    }
    #endif
    lockedAccount[characterId]=0;
    qDebug() << QStringLiteral("lock the char %1 total locked: %2").arg(characterId).arg(lockedAccount.size());
}

//don't apply on InternalGameServer
void CharactersGroup::unlockTheCharacter(const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!lockedAccount.contains(characterId))
        qDebug() << QStringLiteral("try unlonk %1 but already unlocked, relock for 5s").arg(characterId);
    else if(lockedAccount.value(characterId)!=0)
        qDebug() << QStringLiteral("unlock %1 already planned into: %2 (reset for 5s)").arg(characterId).arg(lockedAccount.value(characterId));
    #endif
    lockedAccount[characterId]=QDateTime::currentMSecsSinceEpoch()/1000+5;
    qDebug() << QStringLiteral("unlock the char %1 total locked: %2").arg(characterId).arg(lockedAccount.size());
}

void CharactersGroup::waitBeforeReconnect(const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!lockedAccount.contains(characterId))
        qDebug() << QStringLiteral("lockedAccount not contains: %1").arg(characterId);
    else if(lockedAccount.value(characterId)!=0)
        qDebug() << QStringLiteral("lockedAccount contains set timeout: %1").arg(characterId);
    #endif
    lockedAccount[characterId]=QDateTime::currentMSecsSinceEpoch()/1000+5;
    qDebug() << QStringLiteral("waitBeforeReconnect the char %1 total locked: %2").arg(characterId).arg(lockedAccount.size());
}

void CharactersGroup::purgeTheLockedAccount()
{
    bool clockDriftDetected=false;
    QList<quint32> charactedToUnlock;
    QHashIterator<quint32/*uniqueKey*/,quint64/*can reconnect after this time stamps if !=0, else locked*/> i(lockedAccount);
    while (i.hasNext()) {
        i.next();
        if(i.value()!=0)
        {
            if(i.value()<(quint64)QDateTime::currentMSecsSinceEpoch()/1000)
                charactedToUnlock << i.key();
            else if(i.value()>((quint64)QDateTime::currentMSecsSinceEpoch()/1000)+3600)
            {
                charactedToUnlock << i.key();
                clockDriftDetected=true;
            }
        }
    }
    int index=0;
    while(index<charactedToUnlock.size())
    {
        lockedAccount.remove(charactedToUnlock.at(index));
        index++;
    }
    if(clockDriftDetected)
        qDebug() << QStringLiteral("Some locked account for more than 1h, clock drift?");
    if(charactedToUnlock.isEmpty())
        qDebug() << QStringLiteral("purged char number %1 total locked: %2").arg(charactedToUnlock.size()).arg(lockedAccount.size());
}
