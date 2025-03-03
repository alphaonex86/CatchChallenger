#ifndef CATCHCHALLENGER_SERVER_STRUCTURES_H
#define CATCHCHALLENGER_SERVER_STRUCTURES_H

#if defined(CATCHCHALLENGER_DB_FILE)
    #ifndef CATCHCHALLENGER_CLASS_ALLINONESERVER
        #error for CATCHCHALLENGER_DB_FILE you need enable CATCHCHALLENGER_CLASS_ALLINONESERVER
    #endif
    #ifndef CATCHCHALLENGER_CACHE_HPS
        #error for CATCHCHALLENGER_DB_FILE you need enable CATCHCHALLENGER_CACHE_HPS
    #endif
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        #error for CATCHCHALLENGER_DB_FILE you need disable CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    #endif
    #if defined(CATCHCHALLENGER_DB_BLACKHOLE)
        #error for CATCHCHALLENGER_DB_FILE you need disable CATCHCHALLENGER_DB_BLACKHOLE
    #endif
#endif
#if defined(CATCHCHALLENGER_DB_BLACKHOLE)
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_DB_FILE)
        #error for CATCHCHALLENGER_DB_BLACKHOLE you need disable: CATCHCHALLENGER_DB_MYSQL, CATCHCHALLENGER_DB_POSTGRESQL, CATCHCHALLENGER_DB_SQLITE, CATCHCHALLENGER_DB_FILE
    #endif
#endif

#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <regex>

#include "../../general/base/GeneralStructures.hpp"
#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
#include "../../general/base/CompressionProtocol.hpp"
#endif
#include "PlayerUpdaterBase.hpp"
#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "../game-server-alone/timer/PlayerUpdaterToMaster.hpp"
#endif
#include "../../general/base/GeneralVariable.hpp"
#include "VariableServer.hpp"
#include "../base/PreparedDBQuery.hpp"
#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include <atomic>
#endif

namespace CatchChallenger {
class CommonMap;
class Client;
class MapServer;
class Map_server_MapVisibility_Simple_StoreOnSender;
class PlayerUpdaterBase;

struct Map_player_info
{
    CommonMap *map;
    int x,y;
    std::string skin;
};

struct FileToSend
{
    //not QFile * to prevent too many file open
    std::string file;
};

enum QuestAction : uint8_t
{
    QuestAction_Start,
    QuestAction_Finish,
    QuestAction_Cancel,
    QuestAction_NextStep
};

class PlayerOnMap
{
public:
    CommonMap* map;
    COORD_TYPE x;
    COORD_TYPE y;
    Orientation orientation;
};

struct NormalServerSettings
{
    uint16_t server_port;
    std::string server_ip;
    std::string proxy;
    uint16_t proxy_port;
    bool useSsl;
};

struct LoginServerSettings
{
    bool announce;
};

class GameServerSettings
{
public:
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    CompressionProtocol::CompressionType compressionType;
    #endif
    bool sendPlayerNumber;
    bool anonymous;
    //fight
    bool pvp;
    uint16_t max_players;//not common because if null info not send, if == 1 then internal

    //the listen, implicit on the client
    std::string datapack_basePath;
    bool dontSendPlayerType;
    bool teleportIfMapNotFoundOrOutOfMap;
    bool everyBodyIsRoot;
    int32_t datapackCache;//-1 = disable, 0 = no timeout, else it's the timeout in s

    enum FightSync : uint8_t
    {
        FightSync_AtEachTurn=0x00,
        FightSync_AtTheEndOfBattle=0x01,//or at the object usage
        FightSync_AtTheDisconnexion=0x02
    };
    FightSync fightSync;
    bool positionTeleportSync;
    uint32_t secondToPositionSync;//0 is disabled

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    class Database
    {
    public:
        std::string file;
        std::string host;
        std::string db;
        std::string login;
        std::string pass;

        DatabaseBase::DatabaseType tryOpenType;
        unsigned int tryInterval;//second
        unsigned int considerDownAfterNumberOfTry;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << file << host << db << login << pass << (uint8_t)tryOpenType << tryInterval << considerDownAfterNumberOfTry;
        }
        template <class B>
        void parse(B& buf) {
            uint8_t value=0;
            buf >> file >> host >> db >> login >> pass >> value;
            tryOpenType=(DatabaseBase::DatabaseType)value;
            buf >> tryInterval >> considerDownAfterNumberOfTry;
        }
        #endif
    };
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    Database database_login;
    Database database_base;
    #endif
    Database database_common;
    Database database_server;
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #endif

    //connection
    bool automatic_account_creation;

    //visibility algorithm
    class MapVisibility
    {
    public:
        class MapVisibility_Simple
        {
        public:
            uint16_t max;
            uint16_t reshow;
            bool enable;
            #ifdef CATCHCHALLENGER_CACHE_HPS
            template <class B>
            void serialize(B& buf) const {
                buf << max << reshow << enable;
            }
            template <class B>
            void parse(B& buf) {
                buf >> max >> reshow >> enable;
            }
            #endif
        };
        MapVisibility_Simple simple;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << simple;
        }
        template <class B>
        void parse(B& buf) {
            buf >> simple;
        }
        #endif
    };
    MapVisibility mapVisibility;

    //DDOS algorithm
    class DDOS
    {
    public:
        #ifdef CATCHCHALLENGER_DDOS_FILTER
        uint8_t kickLimitMove;
        uint8_t kickLimitChat;
        uint8_t kickLimitOther;
        #endif
        uint8_t computeAverageValueTimeInterval;
        int dropGlobalChatMessageGeneral;
        int dropGlobalChatMessageLocalClan;
        int dropGlobalChatMessagePrivate;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << kickLimitMove << kickLimitChat << kickLimitOther
                << computeAverageValueTimeInterval << dropGlobalChatMessageGeneral
                << dropGlobalChatMessageLocalClan << dropGlobalChatMessagePrivate;
        }
        template <class B>
        void parse(B& buf) {
            buf >> kickLimitMove >> kickLimitChat >> kickLimitOther
                >> computeAverageValueTimeInterval >> dropGlobalChatMessageGeneral
                >> dropGlobalChatMessageLocalClan >> dropGlobalChatMessagePrivate;
        }
        #endif
    };
    DDOS ddos;

    //city
    City city;

    class ProgrammedEvent
    {
    public:
        std::string value;
        uint16_t cycle;//mins
        uint16_t offset;//mins
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << value << cycle << offset;
        }
        template <class B>
        void parse(B& buf) {
            buf >> value >> cycle >> offset;
        }
        #endif
    };
    std::unordered_map<std::string/*type, example: day*/,std::unordered_map<std::string/*groupName, example: day/night*/,ProgrammedEvent> > programmedEventList;

    char private_token_statclient[TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];

    //content
    std::string server_message;
    std::string daillygift;

#ifdef CATCHCHALLENGER_CACHE_HPS
template <class B>
void serialize(B& buf) const {
    buf << (uint8_t)compressionType;
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    buf << (uint8_t)CompressionProtocol::compressionLevel;
    #else
    buf << (uint8_t)6;
    #endif
    buf << sendPlayerNumber;
    buf << anonymous;
    //fight
    buf << pvp;
    buf << max_players;//not common because if null info not send, if == 1 then internal

    //the listen, implicit on the client
    buf << dontSendPlayerType;
    buf << teleportIfMapNotFoundOrOutOfMap;
    buf << everyBodyIsRoot;
    buf << datapackCache;//-1 = disable, 0 = no timeout, else it's the timeout in s

    buf << (uint8_t)fightSync;
    buf << positionTeleportSync;
    buf << secondToPositionSync;//0 is disabled

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    buf << database_login;
    buf << database_base;
    #endif
    buf << database_common;
    buf << database_server;
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #endif

    //connection
    buf << automatic_account_creation;

    //visibility algorithm
    buf << mapVisibility;

    //DDOS algorithm
    buf << ddos;

    //city
    buf << city;

    buf << programmedEventList;

    buf.write((char *)private_token_statclient,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);

    //content
    buf << server_message;
    buf << daillygift;
}
template <class B>
void parse(B& buf) {
    uint8_t smallTemp=0;
    buf >> smallTemp;
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    compressionType=(CompressionProtocol::CompressionType)smallTemp;
    #endif
    uint8_t tempcompressionLevel=0;
    buf >> tempcompressionLevel;
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    CompressionProtocol::compressionLevel=tempcompressionLevel;
    #endif
    buf >> sendPlayerNumber;
    buf >> anonymous;
    //fight
    buf >> pvp;
    buf >> max_players;//not common because if null info not send, if == 1 then internal

    //the listen, implicit on the client
    buf >> dontSendPlayerType;
    buf >> teleportIfMapNotFoundOrOutOfMap;
    buf >> everyBodyIsRoot;
    buf >> datapackCache;//-1 = disable, 0 = no timeout, else it's the timeout in s

    buf >> smallTemp;
    fightSync=(FightSync)smallTemp;
    buf >> positionTeleportSync;
    buf >> secondToPositionSync;//0 is disabled

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    buf >> database_login;
    buf >> database_base;
    #endif
    buf >> database_common;
    buf >> database_server;
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #endif

    //connection
    buf >> automatic_account_creation;

    //visibility algorithm
    buf >> mapVisibility;

    //DDOS algorithm
    buf >> ddos;

    //city
    buf >> city;

    buf >> programmedEventList;

    buf.read(private_token_statclient,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);

    //content
    buf >> server_message;
    buf >> daillygift;
}
#endif
};

struct CityStatus
{
    uint32_t clan;
};

struct MarketPlayerMonster
{
    PlayerMonster monster;
    uint32_t player;
    uint64_t price;
};

struct MarketItem
{
    uint32_t marketObjectUniqueId;
    uint16_t item;
    uint32_t quantity;
    uint32_t player;
    uint64_t price;
};

struct Clan
{
    uint16_t captureCityInProgress;//65535 no capture in progress
    uint16_t capturedCity;//65535 no city captured
    uint32_t clanId;
    std::vector<Client *> players;

    //the db info
    std::string name;
    uint64_t cash;
};

struct CaptureCityValidated
{
    std::vector<Client *> players;
    std::vector<Client *> playersInFight;
    std::vector<std::pair<CATCHCHALLENGER_TYPE_MAPID/*mapId*/,uint8_t/*botId*/> > bots;
    std::vector<std::pair<CATCHCHALLENGER_TYPE_MAPID/*mapId*/,uint8_t/*botId*/> > botsInFight;
    std::unordered_map<uint32_t,uint16_t> clanSize;
};

struct ServerProfileInternal
{
    CATCHCHALLENGER_TYPE_MAPID mapIndex;
    /*COORD_TYPE*/ uint8_t x;
    /*COORD_TYPE*/ uint8_t y;
    Orientation orientation;

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    struct PreparedStatementForCreationMonsterGroup
    {
        std::vector<PreparedStatementUnit> monster_insert;
        PreparedStatementUnit character_insert;
    };
    struct PreparedStatementForCreationType
    {
        std::vector<PreparedStatementForCreationMonsterGroup> monsterGroup;
    };
    struct PreparedStatementForCreation
    {
        //not needed std::vector<ServerProfileInternal>
        PreparedStatementForCreationType type;
    };
    PreparedStatementForCreation preparedStatementForCreationByCommon;
    #endif

    PreparedStatementUnit preparedQueryAddCharacterForServer;
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    bool valid;
};

struct ServerPrivateVariables
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    //bd
    #if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLIENT) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
    PreparedDBQueryCommonForLogin preparedDBQueryCommonForLogin;
    #endif
    //if change this defined, change it too into PreparedDBQuery.h
    #if defined(CATCHCHALLENGER_CLASS_LOGIN) || defined(CATCHCHALLENGER_CLIENT) || defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
    PreparedDBQueryCommon preparedDBQueryCommon;
    #endif
    #if defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLIENT) || defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_QT)
    PreparedDBQueryServer preparedDBQueryServer;
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif

    PlayerUpdaterBase *player_updater;

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    //bd
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    CatchChallenger::DatabaseBase *db_login;
    CatchChallenger::DatabaseBase *db_base;
    #endif
    CatchChallenger::DatabaseBase *db_common;
    CatchChallenger::DatabaseBase *db_server;//pointer to don't change the code for below preprocessor code
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #endif

    std::vector<ServerProfileInternal> serverProfileInternalList;

    //datapack
    std::string datapack_mapPath;
    std::string mainDatapackFolder;
    std::string subDatapackFolder;
    std::regex datapack_rightFileName;
    std::regex datapack_rightFolderName;

    //fight
    std::unordered_map<uint16_t,std::vector<MonsterDrops> > monsterDrops;
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    std::vector<uint32_t> maxMonsterId;
    std::vector<uint32_t> maxClanId;
    #else
    //for the single player
    #ifdef EPOLLCATCHCHALLENGERSERVER
    unsigned int maxClanId;
        #ifndef CATCHCHALLENGER_DB_FILE
            unsigned int maxMonsterId;
        #endif
    #else
    //why use std::atomic?
    std::atomic<unsigned int> maxClanId;
    std::atomic<unsigned int> maxMonsterId;
    #endif
    #endif
    //to the zone id, see GlobalServerData::serverPrivateVariables.zoneToId
    std::vector<std::vector<CATCHCHALLENGER_TYPE_MAPID/*mapId*/> > zoneIdToMapList;
    std::vector<CityStatus> cityStatusList;
    std::unordered_map<uint32_t,uint16_t> cityStatusListReverse;
    std::unordered_set<uint32_t> tradedMonster;
    std::vector<char> randomData;

    //market
    std::vector<MarketItem> marketItemList;
    std::vector<MarketPlayerMonster> marketPlayerMonsterList;

    //general data
    bool stopIt;
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    uint32_t maxAccountId;
    uint32_t maxCharacterId;
    #endif
    uint64_t time_city_capture;
    std::unordered_map<uint32_t,Clan *> clanList;

    /* MOVED TO CommonMap
     * Server use ServerMap, Client use Common Map
     * Then the pointer don't have fixed size
     * Then can't just use pointer archimectic
     * then Object size save into CommonMap */
    //size set via MapServer::mapListSize, NO holes, map valid and exists, NOT map_list.size() to never load the path
    //Map_server_MapVisibility_Simple_StoreOnSender * flat_map_list;
    //CATCHCHALLENGER_TYPE_MAPID flat_map_size;
    //std::unordered_map<std::string,CommonMap *> map_list;
    //std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,std::string > id_map_to_map;
    //id from BD use DictionaryServer::dictionary_map_database_to_internal -> NULL if not found
    int8_t sizeofInsertRequest;

    //connection
    uint16_t connected_players;
    std::unordered_set<uint32_t> connected_players_id_list;
    std::vector<std::string> server_message;
    struct GiftEntry
    {
        uint32_t min_random_number,max_random_number;
        uint16_t item;
    };
    std::vector<GiftEntry> gift_list;

    uint32_t number_of_bots_logged;
    int botSpawnIndex;
    std::unordered_map<uint32_t,IndustryStatus> industriesStatus;
    std::vector<uint8_t> events;

    //xp rate at form for level to xp: a*exp(x*b+c)+d
    struct Xp
    {
        uint32_t a,b,c,d;
    };
    //xp rate at form for level to xp: a*exp(x*b+c)+d
    struct Sp
    {
        uint32_t a,b,c,d;
    };

    //datapack
    std::unordered_map<std::string,uint8_t> skinList;
};

bool operator==(const CatchChallenger::MonsterDrops &monsterDrops1,const CatchChallenger::MonsterDrops &monsterDrops2);
bool operator<(const CatchChallenger::FileToSend &fileToSend1,const CatchChallenger::FileToSend &fileToSend2);
}

#endif // STRUCTURES_SERVER_H
