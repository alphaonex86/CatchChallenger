#ifndef CATCHCHALLENGER_SERVER_STRUCTURES_H
#define CATCHCHALLENGER_SERVER_STRUCTURES_H

#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <regex>

#include "../../general/base/GeneralStructures.hpp"
#include "PlayerUpdater.hpp"
#include "TimeRangeEventScan.hpp"
#include "StringWithReplacement.hpp"
#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "PlayerUpdaterToMaster.hpp"
#endif
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../VariableServer.hpp"
#include "../base/PreparedDBQuery.hpp"
#ifdef EPOLLCATCHCHALLENGERSERVER
#include "../epoll/timer/TimerCityCapture.hpp"
#include "../epoll/timer/TimerDdos.hpp"
#include "../epoll/timer/TimerPositionSync.hpp"
#include "../epoll/timer/TimerSendInsertMoveRemove.hpp"
#include "../epoll/timer/TimerEvents.hpp"
#include "../base/DatabaseBase.hpp"
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL)
    #include "../epoll/db/EpollPostgresql.hpp"
    #define EpollDatabaseAsync EpollPostgresql
    #elif defined(CATCHCHALLENGER_DB_MYSQL)
    #include "../epoll/db/EpollMySQL.hpp"
    #else
    #error Unknow database type
    #endif
#endif
#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include <atomic>
#endif

#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QTimer>
#include "QtTimerEvents.hpp"
#include "QtDatabase.hpp"
#endif

namespace CatchChallenger {
class EventThreader;
class Map_custom;
class CommonMap;
class ClientMapManagement;
class PlayerUpdater;
class TimeRangeEventScan;
class Map_server_MapVisibility_Simple_StoreOnReceiver;
class Client;
class MapServer;

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

enum MapVisibilityAlgorithmSelection : uint8_t
{
    MapVisibilityAlgorithmSelection_WithBorder,
    MapVisibilityAlgorithmSelection_Simple,
    MapVisibilityAlgorithmSelection_None
};

enum QuestAction : uint8_t
{
    QuestAction_Start,
    QuestAction_Finish,
    QuestAction_Cancel,
    QuestAction_NextStep
};

struct PlayerOnMap
{
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
    CompressionType compressionType;
    uint8_t compressionLevel;
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

    uint8_t common_blobversion_datapack;
    uint8_t server_blobversion_datapack;

    //connection
    bool automatic_account_creation;

    //visibility algorithm
    class MapVisibility
    {
    public:
        MapVisibilityAlgorithmSelection mapVisibilityAlgorithm;

        class MapVisibility_Simple
        {
        public:
            uint16_t max;
            uint16_t reshow;
            bool reemit;
            #ifdef CATCHCHALLENGER_CACHE_HPS
            template <class B>
            void serialize(B& buf) const {
                buf << max << reshow << reemit;
            }
            template <class B>
            void parse(B& buf) {
                buf >> max >> reshow >> reemit;
            }
            #endif
        };
        MapVisibility_Simple simple;
        class MapVisibility_WithBorder
        {
        public:
            uint16_t maxWithBorder;
            uint16_t reshowWithBorder;
            uint16_t max;
            uint16_t reshow;
            #ifdef CATCHCHALLENGER_CACHE_HPS
            template <class B>
            void serialize(B& buf) const {
                buf << maxWithBorder << reshowWithBorder << max << reshow;
            }
            template <class B>
            void parse(B& buf) {
                buf >> maxWithBorder >> reshowWithBorder >> max >> reshow;
            }
            #endif
        };
        MapVisibility_WithBorder withBorder;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << (uint8_t)mapVisibilityAlgorithm << simple << withBorder;
        }
        template <class B>
        void parse(B& buf) {
            uint8_t value=0;
            buf >> value;
            mapVisibilityAlgorithm=(MapVisibilityAlgorithmSelection)value;
            buf >> simple >> withBorder;
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
    buf << compressionLevel;
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

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    buf << database_login;
    buf << database_base;
    #endif
    buf << database_common;
    buf << database_server;

    buf << common_blobversion_datapack;
    buf << server_blobversion_datapack;

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
    compressionType=(CompressionType)smallTemp;
    buf >> compressionLevel;
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

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    buf >> database_login;
    buf >> database_base;
    #endif
    buf >> database_common;
    buf >> database_server;

    buf >> common_blobversion_datapack;
    buf >> server_blobversion_datapack;

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
    #ifndef EPOLLCATCHCHALLENGERSERVER
    std::string captureCityInProgress;
    std::string capturedCity;
    #endif
    uint32_t clanId;
    std::vector<Client *> players;

    //the db info
    std::string name;
    uint64_t cash;
};

#ifndef EPOLLCATCHCHALLENGERSERVER
struct CaptureCityValidated
{
    std::vector<Client *> players;
    std::vector<Client *> playersInFight;
    std::vector<uint16_t> bots;
    std::vector<uint16_t> botsInFight;
    std::unordered_map<uint32_t,uint16_t> clanSize;
};
#endif

struct ServerProfileInternal
{
    MapServer *map;
    /*COORD_TYPE*/ uint8_t x;
    /*COORD_TYPE*/ uint8_t y;
    Orientation orientation;

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
    bool valid;
};

struct ServerPrivateVariables
{
    //bd
    #ifdef EPOLLCATCHCHALLENGERSERVER
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    CatchChallenger::DatabaseBase *db_login;
    CatchChallenger::DatabaseBase *db_base;
    #endif
    CatchChallenger::DatabaseBase *db_common;
    CatchChallenger::DatabaseBase *db_server;//pointer to don't change the code for below preprocessor code
    std::vector<TimerEvents *> timerEvents;
    #else
    QtDatabase *db_login;
    QtDatabase *db_base;
    QtDatabase *db_common;
    QtDatabase *db_server;
    std::vector<QtTimerEvents *> timerEvents;
    #endif
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
    std::atomic<unsigned int> maxClanId;
    std::atomic<unsigned int> maxMonsterId;
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    std::unordered_map<std::string,std::vector<uint16_t> > captureFightIdListByZoneToCaptureCity;
    std::unordered_map<std::string,CityStatus> cityStatusList;
    std::unordered_map<uint32_t,std::string > cityStatusListReverse;
    #endif
    std::unordered_set<uint32_t> tradedMonster;
    std::vector<char> randomData;

    uint8_t common_blobversion_datapack;
    uint8_t server_blobversion_datapack;

    //market
    std::vector<MarketItem> marketItemList;
    std::vector<MarketPlayerMonster> marketPlayerMonsterList;

    //timer and thread
    #ifndef EPOLLCATCHCHALLENGERSERVER
        QTimer *timer_city_capture;
        QTimer *timer_to_send_insert_move_remove;
        QTimer positionSync;
        QTimer ddosTimer;
    #else
        //TimerCityCapture *timer_city_capture;
        TimerDdos ddosTimer;
        TimerPositionSync positionSync;
        TimerSendInsertMoveRemove *timer_to_send_insert_move_remove;
    #endif

    //general data
    bool stopIt;
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    uint32_t maxAccountId;
    uint32_t maxCharacterId;
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    uint64_t time_city_capture;
    #endif
    std::unordered_map<uint32_t,Clan *> clanList;

    //map
    CommonMap ** flat_map_list;//size set via map_list.size() at next line
    std::unordered_map<std::string,CommonMap *> map_list;
    std::unordered_map<uint32_t,std::string > id_map_to_map;
    int8_t sizeofInsertRequest;

    //connection
    uint16_t connected_players;
    PlayerUpdater player_updater;
    TimeRangeEventScan timeRangeEventScan;
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    PlayerUpdaterToMaster player_updater_to_master;
    #endif
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
