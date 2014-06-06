#ifndef CATCHCHALLENGER_SERVER_STRUCTURES_H
#define CATCHCHALLENGER_SERVER_STRUCTURES_H

#include <QObject>
#include <QList>
#include <QStringList>
#include <QString>
#include <QSqlDatabase>
#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QTimer>
#endif
#include <QMutex>
#include <QHash>
#include <QVariant>
#include <QSet>
#include <QSqlQuery>
#include <QDataStream>
#include <QMultiHash>
#include <QRegularExpression>
#include <QDateTime>
#include <QProcess>

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/ConnectedSocket.h"
#include "PlayerUpdater.h"
#include "../../general/base/CommonSettings.h"
#ifdef EPOLLCATCHCHALLENGERSERVER
#include "epoll/TimerCityCapture.h"
#include "epoll/TimerDdos.h"
#include "epoll/TimerPositionSync.h"
#include "epoll/TimerSendInsertMoveRemove.h"
#endif

namespace CatchChallenger {
class EventThreader;
class Map_custom;
class CommonMap;
class ClientBroadCast;
class ClientMapManagement;
class PlayerUpdater;
class Map_server_MapVisibility_Simple_StoreOnReceiver;

struct Map_player_info
{
    CommonMap *map;
    int x,y;
    QString skin;
};

enum MapVisibilityAlgorithmSelection
{
    MapVisibilityAlgorithmSelection_WithBorder,
    MapVisibilityAlgorithmSelection_Simple,
    MapVisibilityAlgorithmSelection_None
};

enum QuestAction
{
    QuestAction_Start,
    QuestAction_Finish,
    QuestAction_Cancel,
    QuestAction_NextStep
};

struct MonsterDrops
{
    quint32 item;
    quint32 quantity_min;
    quint32 quantity_max;
    quint32 luck;//seam be 0 to 100
};

struct Player_internal_informations
{
    Player_private_and_public_informations public_and_private_informations;
    quint32 account_id;//0 if not logged
    quint8 number_of_character;
    bool character_loaded;
    quint32 character_id;
    quint64 market_cash;
    QByteArray rawPseudo;
    volatile bool isConnected;
    struct Rescue
    {
        CommonMap* map;
        COORD_TYPE x;
        COORD_TYPE y;
        Orientation orientation;
    };
    Rescue rescue;
    Rescue unvalidated_rescue;
    QMultiHash<quint32,MonsterDrops> questsDrop;
    QDateTime connectedSince;
    struct OldEvents
    {
        struct OldEventEntry
        {
            quint8 event;
            quint8 eventValue;
        };
        QDateTime time;
        QList<OldEventEntry> oldEventList;
    };
    OldEvents oldEvents;
};

struct NormalServerSettings
{
    quint16 server_port;
    QString server_ip;
    QString proxy;
    quint16 proxy_port;
    bool useSsl;

    struct LinuxSettings
    {
        bool tcpCork;
    };
    LinuxSettings linuxSettings;
};

struct ServerSettings
{
    CompressionType compressionType;
    bool sendPlayerNumber;
    //fight
    bool pvp;
    quint16 max_players;//not common because if null info not send

    //the listen, implicit on the client
    QString datapack_basePath;
    bool anonymous;
    QString server_message;
    bool dontSendPlayerType;
    qint32 datapackCache;//-1 = disable, 0 = no timeout, else it's the timeout in s

    struct Database
    {
        enum Type
        {
            DatabaseType_Mysql,
            DatabaseType_SQLite,
            DatabaseType_PostgreSQL
        };
        Type type;

        enum FightSync
        {
            FightSync_AtEachTurn=0x00,
            FightSync_AtTheEndOfBattle=0x01,//or at the object usage
            FightSync_AtTheDisconnexion=0x02
        };
        FightSync fightSync;
        bool positionTeleportSync;
        quint32 secondToPositionSync;//0 is disabled

        struct Mysql
        {
            //mysql settings
            QString host;
            QString db;
            QString login;
            QString pass;
        };
        Mysql mysql;
        struct SQLite
        {
            QString file;
        };
        SQLite sqlite;
    };
    Database database;

    //connection
    bool tolerantMode;
    bool automatic_account_creation;

    //visibility algorithm
    struct MapVisibility
    {
        MapVisibilityAlgorithmSelection mapVisibilityAlgorithm;

        struct MapVisibility_Simple
        {
            quint16 max;
            quint16 reshow;
            bool storeOnSender;
            bool reemit;
        };
        MapVisibility_Simple simple;
        struct MapVisibility_WithBorder
        {
            quint16 maxWithBorder;
            quint16 reshowWithBorder;
            quint16 max;
            quint16 reshow;
            bool storeOnSender;
        };
        MapVisibility_WithBorder withBorder;
    };
    MapVisibility mapVisibility;

    //visibility algorithm
    struct DDOS
    {
        int computeAverageValueNumberOfValue;
        quint8 computeAverageValueTimeInterval;
        quint8 kickLimitMove;
        quint8 kickLimitChat;
        quint8 kickLimitOther;
        int dropGlobalChatMessageGeneral;
        int dropGlobalChatMessageLocalClan;
        int dropGlobalChatMessagePrivate;
    };
    DDOS ddos;

    //city
    City city;
};

struct CityStatus
{
    quint32 clan;
};

struct MarketPlayerMonster
{
    PlayerMonster monster;
    quint32 player;
    quint32 cash;
};

struct MarketItem
{
    quint32 marketObjectId;
    quint32 item;
    quint32 quantity;
    quint32 player;
    quint32 cash;
};

struct ServerPrivateVariables
{
    //bd
    QSqlDatabase *db;//use pointer here to init in correct thread
    QString db_type_string;

    //query
    QString db_query_login;
    QString db_query_insert_login;
    QString db_query_characters;
    QString db_query_played_time;
    QString db_query_monster_skill;
    QString db_query_monster;
    QString db_query_character_by_id;
    QString db_query_update_character_time_to_delete;
    QString db_query_update_character_last_connect;
    QString db_query_clan;

    QString db_query_monster_by_character_id;
    QString db_query_delete_monster_buff;
    QString db_query_delete_monster_skill;
    QString db_query_delete_bot_already_beaten;
    QString db_query_delete_character;
    QString db_query_delete_item;
    QString db_query_delete_monster;
    QString db_query_delete_plant;
    QString db_query_delete_quest;
    QString db_query_delete_recipes;
    QString db_query_delete_reputation;

    QString db_query_select_character_by_pseudo;
    QString db_query_insert_monster;
    QString db_query_insert_monster_skill;
    QString db_query_insert_reputation;
    QString db_query_insert_item;
    QString db_query_account_time_to_delete_character_by_id;
    QString db_query_update_character_time_to_delete_by_id;
    QString db_query_select_reputation_by_id;
    QString db_query_select_quest_by_id;
    QString db_query_select_recipes_by_player_id;
    QString db_query_select_items_by_player_id;
    QString db_query_delete_item_by_charater_item_place;
    QString db_query_select_monsters_by_player_id;
    QString db_query_select_monstersSkill_by_id;
    QString db_query_select_monstersBuff_by_id;
    QString db_query_select_bot_beaten;
    QString db_query_update_monster_place_wearhouse;

    //datapack
    QString datapack_mapPath;
    QRegularExpression datapack_rightFileName;
    QRegularExpression datapack_rightFolderName;
    QHash<quint32,Shop> shops;

    //fight
    QMultiHash<quint32,MonsterDrops> monsterDrops;
    quint32 maxMonsterId;
    QMutex monsterIdMutex;
    QHash<QString,QList<quint32> > captureFightIdList;
    QHash<QString,CityStatus> cityStatusList;
    QHash<quint32,QString> cityStatusListReverse;
    QSet<quint32> tradedMonster;

    //market
    QList<MarketItem> marketItemList;
    QList<MarketPlayerMonster> marketPlayerMonsterList;

    //timer and thread
    #ifndef EPOLLCATCHCHALLENGERSERVER
        QList<QThread *> eventThreaderList;
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
    quint32 maxClanId;
    quint32 maxAccountId;
    quint32 maxCharacterId;
    QDateTime time_city_capture;

    //datapack
    QHash<QString,quint8> skinList;

    //map
    QHash<QString,CommonMap *> map_list;
    QList<CommonMap *> flat_map_list;
    QHash<quint32,QString> id_map_to_map;
    qint8 sizeofInsertRequest;

    //connection
    quint16 connected_players;
    PlayerUpdater player_updater;
    QSet<quint32> connected_players_id_list;
    QStringList server_message;
    quint16 maxVisiblePlayerAtSameTime;

    quint32 number_of_bots_logged;
    int botSpawnIndex;
    QHash<quint32,IndustryStatus> industriesStatus;
    QList<quint8> events;

    //plant
    QList<quint32> plantUsedId;
    QList<quint32> plantUnusedId;
    quint32 maxPlantId;

    //xp rate at form for level to xp: a*exp(x*b+c)+d
    struct Xp
    {
        quint32 a,b,c,d;
    };
    //xp rate at form for level to xp: a*exp(x*b+c)+d
    struct Sp
    {
        quint32 a,b,c,d;
    };
};

bool operator==(const CatchChallenger::MonsterDrops &monsterDrops1,const CatchChallenger::MonsterDrops &monsterDrops2);
}

#endif // STRUCTURES_SERVER_H
