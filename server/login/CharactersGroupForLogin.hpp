#ifndef CHARACTERSGROUP_H
#define CHARACTERSGROUP_H

#include "../epoll/BaseClassSwitch.hpp"
#include "../epoll/db/EpollPostgresql.hpp"
#include "../base/PreparedDBQuery.hpp"
#include "EpollClientLoginSlave.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace CatchChallenger {
class CharactersGroupForLogin : public BaseClassSwitch
{
public:
    explicit CharactersGroupForLogin(const char * const db,const char * const host,const char * const login,const char * const pass,const uint8_t &considerDownAfterNumberOfTry,const uint8_t &tryInterval);
    ~CharactersGroupForLogin();
    BaseClassSwitch::EpollObjectType getType() const;

    struct InternalGameServer
    {
        std::string host;
        uint16_t port;
        uint8_t indexOnFlatList;
    };

    void clearServerPair();
    void setServerUniqueKey(const uint8_t &indexOnFlatList,const uint32_t &serverUniqueKey,const char * const hostData,const uint8_t &hostDataSize,const uint16_t &port);
    void setIndexServerUniqueKey(const uint8_t &indexOnFlatList,const uint32_t &serverUniqueKey);
    bool containsServerUniqueKey(const uint32_t &serverUniqueKey) const;
    InternalGameServer getServerInformation(const uint32_t &serverUniqueKey) const;
    const std::unordered_map<uint32_t,InternalGameServer> &getServerListRO() const;
    void removeServerUniqueKey(const uint32_t &serverUniqueKey);

    std::vector<EpollClientLoginSlave *> clientQueryForReadReturn;
    std::vector<uint32_t> maxCharacterId;
    std::vector<uint32_t> maxMonsterId;
    bool maxCharacterIdRequested;
    bool maxMonsterIdRequested;
    uint8_t charactersGroupIndex;
    struct AddCharacterParam
    {
        void * client;
        uint8_t query_id;
        uint8_t profileIndex;
        std::string pseudo;
        uint8_t monsterGroupId;
        uint8_t skinId;
    };
    std::vector<AddCharacterParam> addCharacterParamList;
    struct RemoveCharacterParam
    {
        void * client;
        uint8_t query_id;
        uint32_t characterId;
    };
    std::vector<RemoveCharacterParam> removeCharacterParamList;

    static std::unordered_map<std::string,CharactersGroupForLogin *> hash;
    static std::vector<CharactersGroupForLogin *> list;

    void deleteCharacterNow(const uint32_t &characterId);
    static void deleteCharacterNow_static(void *object);
    void deleteCharacterNow_object();
    void deleteCharacterNow_return(const uint32_t &characterId);
    int8_t addCharacter(void * const client,const uint8_t &query_id, const uint8_t &profileIndex, const std::string &pseudo, const uint8_t &monsterGroupId, const uint8_t &skinId);
    static void addCharacterStep1_static(void *object);
    void addCharacterStep1_object();
    void addCharacterStep1_return(EpollClientLoginSlave * const client,const uint8_t &query_id,const uint8_t &profileIndex,const std::string &pseudo, const uint8_t &monsterGroupId,const uint8_t &skinId);
    static void addCharacterStep2_static(void *object);
    void addCharacterStep2_object();
    void addCharacterStep2_return(EpollClientLoginSlave * const client,const uint8_t &query_id,const uint8_t &profileIndex,const std::string &pseudo, const uint8_t &monsterGroupId,const uint8_t &skinId);
    bool removeCharacterLater(void * const client,const uint8_t &query_id, const uint32_t &characterId);
    static void removeCharacterLater_static(void *object);
    void removeCharacterLater_object();
    void removeCharacterLater_return(EpollClientLoginSlave * const client,const uint8_t &query_id,const uint32_t &characterId);

    void character_list(EpollClientLoginSlave * const client,const uint32_t &account_id);
    static void character_list_static(void *object);
    void character_list_object();
    void server_list(EpollClientLoginSlave * const client,const uint32_t &account_id);
    static void server_list_static(void *object);
    void server_list_object();
    DatabaseBase::DatabaseType databaseType() const;
    DatabaseBase * database() const;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    static uint8_t serverCountForAllCharactersGroup();
    #endif
    #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
    static void serverDumpCharactersGroup();
    #endif
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

    void dbQueryWriteCommon(const std::string &queryText);
private:
    EpollPostgresql *databaseBaseCommon;
    PreparedDBQueryCommonForLogin preparedDBQueryCommonForLogin;
    PreparedDBQueryCommon preparedDBQueryCommon;
    std::unordered_map<uint32_t,InternalGameServer> servers;
    std::vector<void *> clientAddReturnList;
    std::vector<void *> clientRemoveReturnList;
    std::vector<uint32_t> deleteCharacterNowCharacterIdList;

    static const std::string gender_male,gender_female;
    static char tempBuffer[4096];
};
}

#endif // CHARACTERSGROUP_H
