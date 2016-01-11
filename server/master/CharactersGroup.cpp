#include "CharactersGroup.h"
#include "EpollServerLoginMaster.h"
#include <iostream>
#include <chrono>

using namespace CatchChallenger;

int CharactersGroup::serverWaitedToBeReady=0;
std::unordered_map<std::string,CharactersGroup *> CharactersGroup::hash;
std::vector<CharactersGroup *> CharactersGroup::list;
uint16_t CharactersGroup::maxLockAge=60;

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
        maxClanId=stringtouint32(databaseBaseCommon->value(0),&ok);
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
        maxCharacterId=stringtouint32(databaseBaseCommon->value(0),&ok);
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
        maxMonsterId=stringtouint32(databaseBaseCommon->value(0),&ok);
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
    if(lockedAccountByDisconnectedServer.find(uniqueKey)!=lockedAccountByDisconnectedServer.cend())
    {
        //new key found on master server
        auto i = lockedAccountByDisconnectedServer.at(uniqueKey).begin();
        while (i != lockedAccountByDisconnectedServer.at(uniqueKey).cend())
        {
            if(lockedAccount.find(*i)==lockedAccount.cend())
            {
                //was disconnected from the last game server disconnection
                this->lockedAccount[*i]=(uint64_t)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()+5;//wait 5s before reconnect
            }
            ++i;
        }
        lockedAccountByDisconnectedServer.erase(uniqueKey);
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
        auto i = lockedAccount.begin();
        while (i != lockedAccount.end())
        {
            const auto thislockAccountIter=this->lockedAccount.find(*i);
            if(thislockAccountIter!=this->lockedAccount.cend())
            {
                const uint64_t &timeLock=this->lockedAccount.at(*i);
                //drop the timeouted lock
                if(timeLock>0 && timeLock<(uint64_t)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count())
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
                        this->lockedAccount[*i]=(uint64_t)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()+5*60;//wait 5min before reconnect
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
    const InternalGameServer &internalGameServer=gameServers.at(uniqueKey);
    lockedAccountByDisconnectedServer[uniqueKey].insert(internalGameServer.lockedAccountByGameserver.cbegin(),internalGameServer.lockedAccountByGameserver.cend());
    gameServers.erase(uniqueKey);
}

bool CharactersGroup::containsGameServerUniqueKey(const uint32_t &serverUniqueKey) const
{
    return gameServers.find(serverUniqueKey)!=gameServers.cend();
}

bool CharactersGroup::characterIsLocked(const uint32_t &characterId)
{
    if(lockedAccount.find(characterId)!=lockedAccount.cend())
    {
        if(lockedAccount.at(characterId)==0)
            return true;
        if(lockedAccount.at(characterId)<(uint64_t)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count())
        {
            lockedAccount.erase(characterId);
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
    if(lockedAccount.find(characterId)!=lockedAccount.cend())
    {
        if(lockedAccount.at(characterId)==0)
        {
            std::cerr << "lockedAccount already contains: " << std::to_string(characterId) << std::endl;
            return;
        }
        if(lockedAccount.at(characterId)>(uint64_t)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count())
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
    //std::cerr << "lock the char " << std::to_string(characterId) << " total locked: " << std::to_string(lockedAccount.size()) << std::endl;
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
    lockedAccount[characterId]=(uint64_t)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()+CharactersGroup::maxLockAge;
    //std::cerr << "unlock the char " << std::to_string(characterId) << " total locked: " << std::to_string(lockedAccount.size()) << std::endl;
}

void CharactersGroup::waitBeforeReconnect(const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(lockedAccount.find(characterId)==lockedAccount.cend())
        std::cerr << "lockedAccount not contains: " << characterId << std::endl;
    else if(lockedAccount.at(characterId)!=0)
        std::cerr << "lockedAccount contains set timeout: " << characterId << std::endl;
    #endif
    lockedAccount[characterId]=(uint64_t)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()+5;
    //std::cerr << "waitBeforeReconnect the char " << std::to_string(characterId) << " total locked: " << std::to_string(lockedAccount.size()) << std::endl;
}

void CharactersGroup::purgeTheLockedAccount()
{
    bool clockDriftDetected=false;
    std::vector<uint32_t> charactedToUnlock;
    auto i=lockedAccount.begin();
    const uint64_t &now=(uint64_t)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    while(i!=lockedAccount.cbegin())
    {
        if(i->second!=0)
        {
            if(i->second<now)
                charactedToUnlock.push_back(i->first);
            else if(i->second>(now+3600))
            {
                i->second=now+CharactersGroup::maxLockAge;
                clockDriftDetected=true;
            }
        }
        ++i;
    }
    unsigned int index=0;
    while(index<charactedToUnlock.size())
    {
        lockedAccount.erase(charactedToUnlock.at(index));
        index++;
    }
    if(clockDriftDetected)
        std::cerr << "Some locked account for more than 1h, clock drift?" << std::endl;
    if(charactedToUnlock.empty())
        std::cerr << "purged char number " << std::to_string(charactedToUnlock.size()) << " total locked: " << std::to_string(lockedAccount.size()) << std::endl;
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
