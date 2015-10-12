#include "CharactersGroup.h"
#include "EpollServerLoginMaster.h"
#include <iostream>
#include <QDateTime>

using namespace CatchChallenger;

int CharactersGroup::serverWaitedToBeReady=0;
std::unordered_map<std::string,CharactersGroup *> CharactersGroup::hash;
std::vector<CharactersGroup *> CharactersGroup::list;

CharactersGroup::CharactersGroup(const char * const db, const char * const host, const char * const login, const char * const pass, const uint8_t &considerDownAfterNumberOfTry, const uint8_t &tryInterval, const std::string &name) :
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
    std::string queryText;
    switch(databaseBaseCommon->databaseType())
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            queryText=QLatin1String("SELECT `id` FROM `clan` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText=QLatin1String("SELECT id FROM clan ORDER BY id DESC LIMIT 0,1;");
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText=QLatin1String("SELECT id FROM clan ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroup::load_clan_max_id_static)==NULL)
    {
        qDebug() << std::stringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
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
        maxClanId=std::string(databaseBaseCommon->value(0)).toUInt(&ok);
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
    std::string queryText;
    switch(databaseBaseCommon->databaseType())
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            queryText=QLatin1String("SELECT `id` FROM `character` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText=QLatin1String("SELECT id FROM character ORDER BY id DESC LIMIT 0,1;");
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText=QLatin1String("SELECT id FROM character ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroup::load_character_max_id_static)==NULL)
    {
        qDebug() << std::stringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
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
        maxCharacterId=std::string(databaseBaseCommon->value(0)).toUInt(&ok);
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
    std::string queryText;
    switch(databaseBaseCommon->databaseType())
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            queryText=QLatin1String("SELECT `id` FROM `monster` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText=QLatin1String("SELECT id FROM monster ORDER BY id DESC LIMIT 0,1;");
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText=QLatin1String("SELECT id FROM monster ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(databaseBaseCommon->asyncRead(queryText.toLatin1(),this,&CharactersGroup::load_monsters_max_id_static)==NULL)
    {
        qDebug() << std::stringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseCommon->errorMessage());
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
        maxMonsterId=std::string(databaseBaseCommon->value(0)).toUInt(&ok);
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

BaseClassSwitch::EpollObjectType CharactersGroup::getType() const
{
    return BaseClassSwitch::EpollObjectType::Client;
}

CharactersGroup::InternalGameServer * CharactersGroup::addGameServerUniqueKey(void * const client, const uint32_t &uniqueKey, const std::string &host,
                                                                              const uint16_t &port, const std::string &metaData, const uint32_t &logicalGroupIndex,
                                                                              const uint16_t &currentPlayer, const uint16_t &maxPlayer,const std::unordered_set<uint32_t> &lockedAccount)
{
    //old locked account
    if(lockedAccountByDisconnectedServer.contains(uniqueKey))
    {
        //new key found on master server
        std::unordered_set<uint32_t>::const_iterator i = lockedAccountByDisconnectedServer.value(uniqueKey).constBegin();
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
    tempServer.host=host;//std::string::fromUtf8(hostData,hostDataSize)
    tempServer.port=port;
    tempServer.link=client;

    tempServer.logicalGroupIndex=logicalGroupIndex;
    tempServer.metaData=metaData;
    tempServer.uniqueKey=uniqueKey;

    tempServer.currentPlayer=currentPlayer;
    tempServer.maxPlayer=maxPlayer;

    //new key found on game server, mostly when the master server is restarted
    {
        std::unordered_set<uint32_t>::const_iterator i = lockedAccount.constBegin();
        while (i != lockedAccount.constEnd()) {
            if(this->lockedAccount.contains(*i))
            {
                const uint64_t &timeLock=this->lockedAccount.value(*i);
                //drop the timeouted lock
                if(timeLock>0 && timeLock<(uint64_t)QDateTime::currentMSecsSinceEpoch()/1000)
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
                        std::unordered_mapIterator<uint32_t/*serverUniqueKey*/,InternalGameServer> j(gameServers);
                        while (j.hasNext()) {
                            j.next();
                            if(j.value().lockedAccount.contains(*i))
                            {
                                static_cast<EpollClientLoginMaster *>(j.value().link)->disconnectForDuplicateConnexionDetected(*i);
                                gameServers[j.key()].lockedAccount.remove(*i);
                            }
                        }
                        this->lockedAccount[*i]=QDateTime::currentMSecsSinceEpoch()/1000+5*60;//wait 5min before reconnect
                        static_cast<EpollClientLoginMaster *>(client)->disconnectForDuplicateConnexionDetected(*i);
                    }
                    else
                    {
                        //recent normal disconnected, just wait the 5s timeout
                        static_cast<EpollClientLoginMaster *>(client)->disconnectForDuplicateConnexionDetected(*i);
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
    EpollClientLoginMaster * const clientCast=static_cast<EpollClientLoginMaster * const>(client);
    clientCast->uniqueKey=uniqueKey;

    return &gameServers[uniqueKey];
}

void CharactersGroup::removeGameServerUniqueKey(void * const client)
{
    EpollClientLoginMaster * const clientCast=static_cast<EpollClientLoginMaster * const>(client);
    const uint32_t &uniqueKey=clientCast->uniqueKey;
    const InternalGameServer &internalGameServer=gameServers.value(uniqueKey);
    lockedAccountByDisconnectedServer.insert(uniqueKey,internalGameServer.lockedAccount);
    gameServers.remove(uniqueKey);
}

bool CharactersGroup::containsGameServerUniqueKey(const uint32_t &serverUniqueKey) const
{
    return gameServers.contains(serverUniqueKey);
}

bool CharactersGroup::characterIsLocked(const uint32_t &characterId)
{
    if(lockedAccount.contains(characterId))
    {
        if(lockedAccount.value(characterId)==0)
            return true;
        if(lockedAccount.value(characterId)<(uint64_t)QDateTime::currentMSecsSinceEpoch()/1000)
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
void CharactersGroup::lockTheCharacter(const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(lockedAccount.contains(characterId))
    {
        if(lockedAccount.contains(characterId)==0)
        {
            qDebug() << std::stringLiteral("lockedAccount already contains: %1").arg(characterId);
            return;
        }
        if(lockedAccount.contains(characterId)>QDateTime::currentMSecsSinceEpoch()/1000)
        {
            qDebug() << std::stringLiteral("lockedAccount already contains not finished timeout: %1").arg(characterId);
            return;
        }
    }
    std::unordered_mapIterator<uint32_t/*serverUniqueKey*/,InternalGameServer> j(gameServers);
    while (j.hasNext()) {
        j.next();
        if(j.value().lockedAccount.contains(characterId))
        {
            qDebug() << std::stringLiteral("lockedAccount already contains on a game server: %1").arg(characterId);
            return;
        }
    }
    #endif
    lockedAccount[characterId]=0;
    qDebug() << std::stringLiteral("lock the char %1 total locked: %2").arg(characterId).arg(lockedAccount.size());
}

//don't apply on InternalGameServer
void CharactersGroup::unlockTheCharacter(const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!lockedAccount.contains(characterId))
        qDebug() << std::stringLiteral("try unlonk %1 but already unlocked, relock for 5s").arg(characterId);
    else if(lockedAccount.value(characterId)!=0)
        qDebug() << std::stringLiteral("unlock %1 already planned into: %2 (reset for 5s)").arg(characterId).arg(lockedAccount.value(characterId));
    #endif
    lockedAccount[characterId]=QDateTime::currentMSecsSinceEpoch()/1000+5;
    qDebug() << std::stringLiteral("unlock the char %1 total locked: %2").arg(characterId).arg(lockedAccount.size());
}

void CharactersGroup::waitBeforeReconnect(const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!lockedAccount.contains(characterId))
        qDebug() << std::stringLiteral("lockedAccount not contains: %1").arg(characterId);
    else if(lockedAccount.value(characterId)!=0)
        qDebug() << std::stringLiteral("lockedAccount contains set timeout: %1").arg(characterId);
    #endif
    lockedAccount[characterId]=QDateTime::currentMSecsSinceEpoch()/1000+5;
    qDebug() << std::stringLiteral("waitBeforeReconnect the char %1 total locked: %2").arg(characterId).arg(lockedAccount.size());
}

void CharactersGroup::purgeTheLockedAccount()
{
    bool clockDriftDetected=false;
    std::vector<uint32_t> charactedToUnlock;
    std::unordered_mapIterator<uint32_t/*uniqueKey*/,uint64_t/*can reconnect after this time stamps if !=0, else locked*/> i(lockedAccount);
    while (i.hasNext()) {
        i.next();
        if(i.value()!=0)
        {
            if(i.value()<(uint64_t)QDateTime::currentMSecsSinceEpoch()/1000)
                charactedToUnlock << i.key();
            else if(i.value()>((uint64_t)QDateTime::currentMSecsSinceEpoch()/1000)+3600)
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
        qDebug() << std::stringLiteral("Some locked account for more than 1h, clock drift?");
    if(charactedToUnlock.isEmpty())
        qDebug() << std::stringLiteral("purged char number %1 total locked: %2").arg(charactedToUnlock.size()).arg(lockedAccount.size());
}
