#ifndef CHARACTERSGROUP_H
#define CHARACTERSGROUP_H

#include "../epoll/BaseClassSwitch.h"
#include "../epoll/db/EpollPostgresql.h"
#include "EpollClientLoginSlave.h"
#include <std::string>
#include <std::unordered_map>
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
    bool containsServerUniqueKey(const uint32_t &serverUniqueKey) const;
    InternalGameServer getServerInformation(const uint32_t &serverUniqueKey) const;

    QList<EpollClientLoginSlave *> clientQueryForReadReturn;
    std::vector<uint32_t> maxCharacterId;
    std::vector<uint32_t> maxMonsterId;
    uint8_t index;
    struct AddCharacterParam
    {
        void * client;
        uint8_t query_id;
        uint8_t profileIndex;
        std::string pseudo;
        uint8_t skinId;
    };
    QList<AddCharacterParam> addCharacterParamList;
    struct RemoveCharacterParam
    {
        void * client;
        uint8_t query_id;
        uint32_t characterId;
    };
    QList<RemoveCharacterParam> removeCharacterParamList;

    static std::unordered_map<std::string,CharactersGroupForLogin *> hash;
    static QList<CharactersGroupForLogin *> list;

    void deleteCharacterNow(const uint32_t &characterId);
    static void deleteCharacterNow_static(void *object);
    void deleteCharacterNow_object();
    void deleteCharacterNow_return(const uint32_t &characterId);
    int8_t addCharacter(void * const client,const uint8_t &query_id, const uint8_t &profileIndex, const std::string &pseudo, const uint8_t &skinId);
    static void addCharacterStep1_static(void *object);
    void addCharacterStep1_object();
    void addCharacterStep1_return(EpollClientLoginSlave * const client,const uint8_t &query_id,const uint8_t &profileIndex,const std::string &pseudo,const uint8_t &skinId);
    static void addCharacterStep2_static(void *object);
    void addCharacterStep2_object();
    void addCharacterStep2_return(EpollClientLoginSlave * const client,const uint8_t &query_id,const uint8_t &profileIndex,const std::string &pseudo,const uint8_t &skinId);
    bool removeCharacter(void * const client,const uint8_t &query_id, const uint32_t &characterId);
    static void removeCharacter_static(void *object);
    void removeCharacter_object();
    void removeCharacter_return(EpollClientLoginSlave * const client,const uint8_t &query_id,const uint32_t &characterId);

    void character_list(EpollClientLoginSlave * const client,const uint32_t &account_id);
    static void character_list_static(void *object);
    void character_list_object();
    void server_list(EpollClientLoginSlave * const client,const uint32_t &account_id);
    static void server_list_static(void *object);
    void server_list_object();
    DatabaseBase::DatabaseType databaseType() const;
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

    void dbQueryWriteCommon(const char * const queryText);
private:
    EpollPostgresql *databaseBaseCommon;
    std::unordered_map<uint32_t,InternalGameServer> servers;
    QList<void * const> clientAddReturnList;
    QList<void * const> clientRemoveReturnList;
    QList<uint32_t> deleteCharacterNowCharacterIdList;
    std::unordered_map<uint32_t,uint8_t> uniqueKeyToIndex;

    static char tempBuffer[4096];
};
}

#endif // CHARACTERSGROUP_H
