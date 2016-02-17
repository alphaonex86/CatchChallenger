#ifndef CHARACTERSGROUP_H
#define CHARACTERSGROUP_H

#include "../epoll/BaseClassSwitch.h"
#include "../epoll/db/EpollPostgresql.h"
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace CatchChallenger {
class EpollClientLoginMaster;

class CharactersGroup : public BaseClassSwitch
{
public:
    explicit CharactersGroup(const char * const db, const char * const host, const char * const login, const char * const pass, const uint8_t &considerDownAfterNumberOfTry, const uint8_t &tryInterval, const std::string &name);
    ~CharactersGroup();

    struct InternalGameServer
    {
        std::string host;
        uint16_t port;
        EpollClientLoginMaster * link;

        //stored into EpollClientLoginMaster
        //CharactersGroup * charactersGroup;
        uint32_t uniqueKey;
        std::string metaData;
        uint32_t logicalGroupIndex;

        uint16_t currentPlayer;
        uint16_t maxPlayer;

        std::unordered_set<uint32_t> lockedAccountByGameserver;

        bool pingInProgress;
        uint64_t lastPingStarted;
    };
    enum CharacterLock : std::uint8_t
    {
        Unlocked=0x00,
        Locked=0x01,
        RecentlyUnlocked=0x02
    };

    BaseClassSwitch::EpollObjectType getType() const;
    InternalGameServer * addGameServerUniqueKey(EpollClientLoginMaster * const client, const uint32_t &uniqueKey, const std::string &host, const uint16_t &port,
                                const std::string &metaData, const uint32_t &logicalGroupIndex,
                                const uint16_t &currentPlayer, const uint16_t &maxPlayer, const std::unordered_set<uint32_t> &lockedAccount);
    void removeGameServerUniqueKey(void * const client);
    bool containsGameServerUniqueKey(const uint32_t &serverUniqueKey) const;
    CharacterLock characterIsLocked(const uint32_t &characterId);
    //need check if is already locked before this call
    //don't apply on InternalGameServer
    void lockTheCharacter(const uint32_t &characterId);
    //don't apply on InternalGameServer
    void unlockTheCharacter(const uint32_t &characterId);
    void waitBeforeReconnect(const uint32_t &characterId);
    void purgeTheLockedAccount();
    void setMaxLockAge(const uint16_t &maxLockAge);

    uint32_t maxClanId;
    uint32_t maxCharacterId;
    uint32_t maxMonsterId;

    std::unordered_map<uint32_t/*serverUniqueKey*/,InternalGameServer> gameServers;

    static uint64_t lastPingStarted;
    static int serverWaitedToBeReady;
    static std::unordered_map<std::string,CharactersGroup *> hash;
    static std::vector<CharactersGroup *> list;
    static uint16_t gameserverTimeoutms;
    static uint32_t pingMSecond;
    std::string name;
    uint8_t index;
private:
    void load_character_max_id();
    static void load_character_max_id_static(void *object);
    void load_character_max_id_return();
    void load_monsters_max_id();
    static void load_monsters_max_id_static(void *object);
    void load_monsters_max_id_return();
    void load_clan_max_id();
    static void load_clan_max_id_static(void *object);
    void load_clan_max_id_return();
    void addToCacheLockToDelete(const uint32_t &uniqueKey,const uint64_t &timestampsToDelete);
    void deleteToCacheLockToDelete(const uint32_t &uniqueKey);
private:
    static uint16_t maxLockAge;
    EpollPostgresql *databaseBaseCommon;
    std::unordered_map<uint32_t/*uniqueKey*/,uint64_t/*can reconnect after this time stamps if !=0, else locked*/> lockedAccount;
    std::unordered_map<uint32_t/*uniqueKey*/,std::unordered_set<uint32_t/*lockedAccount*/> > lockedAccountByDisconnectedServer;

    struct CacheLockToDelete
    {
        uint32_t uniqueKey;
        uint64_t timestampsToDelete;
    };
    std::vector<CacheLockToDelete> cacheLockToDeleteList;//presume cacheLockToDeleteList size remain small
};
}

#endif // CHARACTERSGROUP_H
