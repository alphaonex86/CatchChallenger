#include "CharactersGroup.h"
#include "EpollServerLoginMaster.h"
#include <iostream>
#include <chrono>

using namespace CatchChallenger;

uint64_t CharactersGroup::lastPingStarted=0;
int CharactersGroup::serverWaitedToBeReady=0;
std::unordered_map<std::string,CharactersGroup *> CharactersGroup::hash;
std::vector<CharactersGroup *> CharactersGroup::list;
uint16_t CharactersGroup::maxLockAge=60;
uint16_t CharactersGroup::gameserverTimeoutms=1000;
uint32_t CharactersGroup::pingMSecond=60*1000;
uint32_t CharactersGroup::checkTimeoutGameServerMSecond=1000;

CharactersGroup::CharactersGroup(const char * const db, const char * const host, const char * const login, const char * const pass, const uint8_t &considerDownAfterNumberOfTry, const uint8_t &tryInterval, const std::string &name) :
    databaseBaseCommon(new EpollPostgresql())
{
    this->index=0;
    this->name=name;
    databaseBaseCommon->considerDownAfterNumberOfTry=considerDownAfterNumberOfTry;
    databaseBaseCommon->tryInterval=tryInterval;
    if(!databaseBaseCommon->syncConnect(host,db,login,pass))
    {
        std::cerr << "Connect to common database failed:" << databaseBaseCommon->errorMessage() << ", host: " << host << ", db: " << db << ", login: " << login << std::endl;
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
            queryText="SELECT `id` FROM `clan` ORDER BY `id` DESC LIMIT 0,1;";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id FROM clan ORDER BY id DESC LIMIT 0,1;";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id FROM clan ORDER BY id DESC LIMIT 1;";
        break;
    }
    if(databaseBaseCommon->asyncRead(queryText,this,&CharactersGroup::load_clan_max_id_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseCommon->errorMessage() << std::endl;
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
        maxClanId=databaseBaseCommon->stringtouint32(databaseBaseCommon->value(0),&ok);
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
            queryText="SELECT `id` FROM `character` ORDER BY `id` DESC LIMIT 0,1;";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id FROM character ORDER BY id DESC LIMIT 0,1;";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id FROM character ORDER BY id DESC LIMIT 1;";
        break;
    }
    if(databaseBaseCommon->asyncRead(queryText,this,&CharactersGroup::load_character_max_id_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseCommon->errorMessage() << std::endl;
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
        maxCharacterId=databaseBaseCommon->stringtouint32(databaseBaseCommon->value(0),&ok);
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
            queryText="SELECT `id` FROM `monster` ORDER BY `id` DESC LIMIT 0,1;";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id FROM monster ORDER BY id DESC LIMIT 0,1;";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id FROM monster ORDER BY id DESC LIMIT 1;";
        break;
    }
    if(databaseBaseCommon->asyncRead(queryText,this,&CharactersGroup::load_monsters_max_id_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseCommon->errorMessage() << std::endl;
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
        maxMonsterId=databaseBaseCommon->stringtouint32(databaseBaseCommon->value(0),&ok);
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

CharactersGroup::InternalGameServer * CharactersGroup::addGameServerUniqueKey(EpollClientLoginMaster * const client, const uint32_t &uniqueKey, const std::string &host,
                                                                              const uint16_t &port, const std::string &metaData, const uint32_t &logicalGroupIndex,
                                                                              const uint16_t &currentPlayer, const uint16_t &maxPlayer,const std::unordered_set<uint32_t> &lockedAccount)
{
    const auto &now=sFrom1970();
    //old locked account
    if(lockedAccountByDisconnectedServer.find(uniqueKey)!=lockedAccountByDisconnectedServer.cend())
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "lockedAccountByDisconnectedServer for this server have account locked: " << lockedAccountByDisconnectedServer.at(uniqueKey).size() << std::endl;
        #endif
        //new key found on master server
        auto i = lockedAccountByDisconnectedServer.at(uniqueKey).begin();
        while (i != lockedAccountByDisconnectedServer.at(uniqueKey).cend())
        {
            if(lockedAccount.find(*i)==lockedAccount.cend())
            {
                //was disconnected from the last game server disconnection
                this->lockedAccount[*i]=now+5;//wait 5s before reconnect
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                std::cerr << "not more locked on gameserver, database id: " << *i << std::endl;
                #endif
            }
            ++i;
        }
        lockedAccountByDisconnectedServer.erase(uniqueKey);
    }
    InternalGameServer tempServer;
    tempServer.host=host;//std::string::fromUtf8(hostData,hostDataSize)
    tempServer.port=port;
    tempServer.link=client;

    tempServer.lastPingStarted=msFrom1970();
    tempServer.pingInProgress=false;

    tempServer.logicalGroupIndex=logicalGroupIndex;
    tempServer.metaData=metaData;
    tempServer.uniqueKey=uniqueKey;

    tempServer.currentPlayer=currentPlayer;
    tempServer.maxPlayer=maxPlayer;

    //new key found on game server, mostly when the master server is restarted
    {
        auto i = lockedAccount.begin();
        while (i != lockedAccount.end())
        {
            const auto thislockAccountIter=this->lockedAccount.find(*i);
            if(thislockAccountIter!=this->lockedAccount.cend())
            {
                const uint64_t &timeLock=this->lockedAccount.at(*i);
                //drop the timeouted lock
                if(timeLock>0 && timeLock<now)
                {
                    tempServer.lockedAccountByGameserver.insert(*i);
                    this->lockedAccount[*i]=0;
                }
                /// \todo already locked, diconnect all to be more safe, but bug here! network split? block during 5min this character login
                else
                {
                    //find the other game server and disconnect character on it
                    if(Q_UNLIKELY(timeLock==0))
                    {
                        auto j=gameServers.begin();
                        while(j!=gameServers.cend())
                        {
                            if(j->second.lockedAccountByGameserver.find(*i)!=j->second.lockedAccountByGameserver.cend())
                            {
                                static_cast<EpollClientLoginMaster *>(j->second.link)->disconnectForDuplicateConnexionDetected(*i);
                                gameServers[j->first].lockedAccountByGameserver.erase(*i);
                            }
                            ++j;
                        }
                        this->lockedAccount[*i]=now+CharactersGroup::maxLockAge;//wait 5min before reconnect
                        addToCacheLockToDelete(*i,now+CharactersGroup::maxLockAge);
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
                tempServer.lockedAccountByGameserver.insert(*i);
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

    if(gameServers.find(uniqueKey)==gameServers.cend())
    {
        std::cerr << "gameServers not found with unique key: " << uniqueKey << std::endl;
        return;
    }

    const InternalGameServer &internalGameServer=gameServers.at(uniqueKey);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    std::cerr << "removeGameServerUniqueKey with account locked: " << internalGameServer.lockedAccountByGameserver.size() << std::endl;
    #endif
    lockedAccountByDisconnectedServer[uniqueKey].insert(internalGameServer.lockedAccountByGameserver.cbegin(),internalGameServer.lockedAccountByGameserver.cend());
    gameServers.erase(uniqueKey);
}

bool CharactersGroup::containsGameServerUniqueKey(const uint32_t &serverUniqueKey) const
{
    return gameServers.find(serverUniqueKey)!=gameServers.cend();
}

CharactersGroup::CharacterLock CharactersGroup::characterIsLocked(const uint32_t &characterId)
{
    if(lockedAccount.find(characterId)!=lockedAccount.cend())
    {
        const uint64_t timeStamps=lockedAccount.at(characterId);
        if(timeStamps==0)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            auto i=gameServers.begin();
            while(i!=gameServers.cend())
            {
                const InternalGameServer &internalGameServer=i->second;
                if(internalGameServer.lockedAccountByGameserver.find(characterId)!=internalGameServer.lockedAccountByGameserver.cend())
                {
                    std::cerr << "lockedAccount: " << characterId << ", on server: " << internalGameServer.uniqueKey << " " << internalGameServer.host << " " << internalGameServer.port << std::endl;
                    break;
                }
                ++i;
            }
            if(i==gameServers.cend())
                std::cerr << "lockedAccount: " << characterId << std::endl;
            #endif
            return CharactersGroup::CharacterLock::Locked;
        }
        const auto &varsFrom1970=sFrom1970();
        if(timeStamps<varsFrom1970)
        {
            lockedAccount.erase(characterId);
            deleteToCacheLockToDelete(characterId);
            return CharactersGroup::CharacterLock::Unlocked;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            std::cerr << "lockedAccount: " << characterId << ", beacause timeStamps: " << timeStamps << " >= varsFrom1970: " << varsFrom1970 << std::endl;
            #endif
            return CharactersGroup::CharacterLock::RecentlyUnlocked;
        }
    }
    else
        return CharactersGroup::CharacterLock::Unlocked;
}

//need check if is already locked before this call
//don't apply on InternalGameServer
void CharactersGroup::lockTheCharacter(const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(lockedAccount.find(characterId)!=lockedAccount.cend())
    {
        if(lockedAccount.at(characterId)==0)
        {
            std::cerr << "lockedAccount already contains: " << std::to_string(characterId) << std::endl;
            return;
        }
        if(lockedAccount.at(characterId)>sFrom1970())
        {
            std::cerr << "lockedAccount already contains not finished timeout: " << std::to_string(characterId) << std::endl;
            return;
        }
    }
    auto j=gameServers.begin();
    while(j!=gameServers.cend())
    {
        if(j->second.lockedAccountByGameserver.find(characterId)!=j->second.lockedAccountByGameserver.cend())
        {
            std::cerr << "lockedAccount already contains on a game server: " << std::to_string(characterId) << std::endl;
            return;
        }
        ++j;
    }
    #endif
    lockedAccount[characterId]=0;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    std::cerr << "lock the char " << std::to_string(characterId) << " total locked: " << std::to_string(lockedAccount.size()) << std::endl;
    #endif
}

//don't apply on InternalGameServer
void CharactersGroup::unlockTheCharacter(const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(lockedAccount.find(characterId)==lockedAccount.cend())
        std::cerr << "try unlonk " << characterId << " but already unlocked, relock for 5s" << std::endl;
    else if(lockedAccount.at(characterId)!=0)
        std::cerr << "unlock " << characterId << " already planned into: " << lockedAccount.at(characterId) << " (reset for 5s)" << std::endl;
    #endif
    const uint64_t &now=sFrom1970();
    lockedAccount[characterId]=now+CharactersGroup::maxLockAge;
    addToCacheLockToDelete(characterId,now+CharactersGroup::maxLockAge);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    std::cerr << "unlock the char " << std::to_string(characterId) << " total locked: " << std::to_string(lockedAccount.size()) << std::endl;
    #endif
}

void CharactersGroup::waitBeforeReconnect(const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(lockedAccount.find(characterId)==lockedAccount.cend())
        std::cerr << "lockedAccount not contains: " << characterId << std::endl;
    else if(lockedAccount.at(characterId)!=0)
        std::cerr << "lockedAccount contains set timeout: " << characterId << std::endl;
    #endif
    const uint64_t &now=sFrom1970();
    lockedAccount[characterId]=now+5;
    addToCacheLockToDelete(characterId,now+CharactersGroup::maxLockAge);
    //std::cerr << "waitBeforeReconnect the char " << std::to_string(characterId) << " total locked: " << std::to_string(lockedAccount.size()) << std::endl;
}

void CharactersGroup::purgeTheLockedAccount()
{
    if(cacheLockToDeleteList.empty())
        return;

    const uint64_t &now=sFrom1970();

    //time drift
    if(cacheLockToDeleteList.front().timestampsToDelete>(now+3600))
    {
        uint64_t newDate=now+CharactersGroup::maxLockAge;
        unsigned int index=0;
        while(index<cacheLockToDeleteList.size())
        {
            CacheLockToDelete &cacheLockToDelete=cacheLockToDeleteList.at(index);
            cacheLockToDelete.timestampsToDelete=newDate;
            index++;
        }
        std::cerr << "Some locked account for more than 1h, clock drift?" << std::endl;
        return;
    }

    unsigned int index=0;
    while(index<cacheLockToDeleteList.size())
    {
        const CacheLockToDelete &cacheLockToDelete=cacheLockToDeleteList.at(index);
        if(cacheLockToDelete.timestampsToDelete>now)
            break;
        lockedAccount.erase(cacheLockToDelete.uniqueKey);
        index++;
    }
    if(index>=cacheLockToDeleteList.size())
    {
        std::cout << "purged char number: " << std::to_string(cacheLockToDeleteList.size()) << std::endl;
        cacheLockToDeleteList.clear();
    }
    else if(index<cacheLockToDeleteList.size() && index>0)
    {
        cacheLockToDeleteList.erase(cacheLockToDeleteList.cbegin(),cacheLockToDeleteList.cbegin()+index);
        std::cout << "purged char number " << std::to_string(index) << ", total locked: " << std::to_string(lockedAccount.size()) << ", remaining time locked: " << std::to_string(cacheLockToDeleteList.size()) << std::endl;
    }
}

void CharactersGroup::setMaxLockAge(const uint16_t &maxLockAge)
{
    if(maxLockAge<1)
    {
        std::cerr << "maxLockAge can't be <1s" << std::endl;
        return;
    }
    if(maxLockAge>3600)
    {
        std::cerr << "maxLockAge can't be >1h" << std::endl;
        return;
    }
    CharactersGroup::maxLockAge=maxLockAge;
}

void CharactersGroup::addToCacheLockToDelete(const uint32_t &uniqueKey,const uint64_t &timestampsToDelete)
{
    CacheLockToDelete cacheLockToDelete;
    cacheLockToDelete.uniqueKey=uniqueKey;
    cacheLockToDelete.timestampsToDelete=timestampsToDelete;
    cacheLockToDeleteList.push_back(cacheLockToDelete);
}

void CharactersGroup::deleteToCacheLockToDelete(const uint32_t &uniqueKey)
{
    unsigned int index=0;
    while(index<cacheLockToDeleteList.size())
    {
        const CacheLockToDelete &cacheLockToDelete=cacheLockToDeleteList.at(index);
        if(cacheLockToDelete.uniqueKey==uniqueKey)
        {
            cacheLockToDeleteList.erase(cacheLockToDeleteList.cbegin()+index);
            return;
        }
        index++;
    }
}
