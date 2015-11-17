#ifndef CATCHCHALLENGER_SERVER_STRUCTURES_H
#define CATCHCHALLENGER_SERVER_STRUCTURES_H

#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QTimer>
#endif

#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <regex>

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/ConnectedSocket.h"
#include "PlayerUpdater.h"
#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "PlayerUpdaterToMaster.h"
#endif
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"
#ifdef EPOLLCATCHCHALLENGERSERVER
#include "epoll/timer/TimerCityCapture.h"
#include "epoll/timer/TimerDdos.h"
#include "epoll/timer/TimerPositionSync.h"
#include "epoll/timer/TimerSendInsertMoveRemove.h"
#include "epoll/timer/TimerEvents.h"
#include "../base/DatabaseBase.h"
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL)
    #include "epoll/db/EpollPostgresql.h"
    #define EpollDatabaseAsync EpollPostgresql
    #elif defined(CATCHCHALLENGER_DB_MYSQL)
    #include "epoll/db/EpollMySQL.h"
    #else
    #error Unknow database type
    #endif
#else
#include "QtTimerEvents.h"
#include "QtDatabase.h"
#endif
#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include <atomic>
#endif

namespace CatchChallenger {
class EventThreader;
class Map_custom;
class CommonMap;
class ClientMapManagement;
class PlayerUpdater;
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

struct MonsterDrops
{
    uint32_t item;
    uint32_t quantity_min;
    uint32_t quantity_max;
    uint32_t luck;//seam be 0 to 100
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
    #ifdef __linux__
    bool tcpNodelay;
    #endif
};

struct LoginServerSettings
{
    bool announce;
};

struct GameServerSettings
{
    CompressionType compressionType;
    uint8_t compressionLevel;
    bool sendPlayerNumber;
    bool anonymous;
    bool benchmark;
    //fight
    bool pvp;
    uint16_t max_players;//not common because if null info not send, if == 1 then internal

    //the listen, implicit on the client
    std::string datapack_basePath;
    std::string server_message;
    bool dontSendPlayerType;
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

    struct Database
    {
        std::string file;
        std::string host;
        std::string db;
        std::string login;
        std::string pass;

        DatabaseBase::DatabaseType tryOpenType;
        unsigned int tryInterval;//second
        unsigned int considerDownAfterNumberOfTry;
    };
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    Database database_login;
    #endif
    Database database_base;
    Database database_common;
    Database database_server;

    //connection
    bool automatic_account_creation;

    //visibility algorithm
    struct MapVisibility
    {
        MapVisibilityAlgorithmSelection mapVisibilityAlgorithm;

        struct MapVisibility_Simple
        {
            uint16_t max;
            uint16_t reshow;
            bool reemit;
        };
        MapVisibility_Simple simple;
        struct MapVisibility_WithBorder
        {
            uint16_t maxWithBorder;
            uint16_t reshowWithBorder;
            uint16_t max;
            uint16_t reshow;
        };
        MapVisibility_WithBorder withBorder;
    };
    MapVisibility mapVisibility;

    //DDOS algorithm
    struct DDOS
    {
        #ifdef CATCHCHALLENGER_DDOS_FILTER
        uint8_t kickLimitMove;
        uint8_t kickLimitChat;
        uint8_t kickLimitOther;
        #endif
        int computeAverageValueNumberOfValue;
        uint8_t computeAverageValueTimeInterval;
        int dropGlobalChatMessageGeneral;
        int dropGlobalChatMessageLocalClan;
        int dropGlobalChatMessagePrivate;
    };
    DDOS ddos;

    //city
    City city;

    struct ProgrammedEvent
    {
        std::string value;
        uint16_t cycle;//mins
        uint16_t offset;//mins
    };
    std::unordered_map<std::string/*type, example: day*/,std::unordered_map<std::string/*groupName, example: day/night*/,ProgrammedEvent> > programmedEventList;
};

struct CityStatus
{
    uint32_t clan;
};

struct MarketPlayerMonster
{
    PlayerMonster monster;
    uint32_t player;
    uint32_t cash;
};

struct MarketItem
{
    uint32_t marketObjectId;
    uint32_t item;
    uint32_t quantity;
    uint32_t player;
    uint32_t cash;
};

struct Clan
{
    std::string captureCityInProgress;
    std::string capturedCity;
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
    std::vector<uint16_t> bots;
    std::vector<uint16_t> botsInFight;
    std::unordered_map<uint32_t,uint16_t> clanSize;
};

struct ServerProfileInternal
{
    MapServer *map;
    /*COORD_TYPE*/ uint8_t x;
    /*COORD_TYPE*/ uint8_t y;
    Orientation orientation;

    /*struct MonsterProfile
    {
        std::vector<std::string > monster;
        std::vector<std::vector<std::string > > preparedQueryAddMonsterSkill;
    };*/
    //only to add
    std::vector<std::string > preparedQueryAddCharacter;
    std::vector<std::string > preparedQueryAddCharacterForServer;
    /*std::vector<MonsterProfile> preparedQueryAddMonster;
    std::vector<std::vector<std::string > > preparedQueryAddItem;
    std::vector<std::vector<std::string > > preparedQueryAddReputation;*/
    bool valid;
};

struct ServerPrivateVariables
{
    //bd
    #ifdef EPOLLCATCHCHALLENGERSERVER
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    CatchChallenger::DatabaseBase *db_login;
    #endif
    CatchChallenger::DatabaseBase *db_base;
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
    std::atomic<unsigned int> maxClanId;
    std::atomic<unsigned int> maxMonsterId;
    #endif
    std::unordered_map<std::string,std::vector<uint16_t> > captureFightIdListByZoneToCaptureCity;
    std::unordered_map<std::string,CityStatus> cityStatusList;
    std::unordered_map<uint32_t,std::string > cityStatusListReverse;
    std::unordered_set<uint32_t> tradedMonster;
    std::vector<char> randomData;

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
    uint64_t time_city_capture;
    std::unordered_map<uint32_t,Clan *> clanList;

    //map
    std::unordered_map<std::string,CommonMap *> map_list;
    CommonMap ** flat_map_list;
    std::unordered_map<uint32_t,std::string > id_map_to_map;
    int8_t sizeofInsertRequest;

    //connection
    uint16_t connected_players;
    PlayerUpdater player_updater;
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    PlayerUpdaterToMaster player_updater_to_master;
    #endif
    std::unordered_set<uint32_t> connected_players_id_list;
    std::vector<std::string > server_message;

    uint32_t number_of_bots_logged;
    int botSpawnIndex;
    std::unordered_map<uint32_t,IndustryStatus> industriesStatus;
    std::vector<uint8_t> events;

    //plant
    std::vector<uint32_t> plantUsedId;
    std::vector<uint32_t> plantUnusedId;
    uint32_t maxPlantId;

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
