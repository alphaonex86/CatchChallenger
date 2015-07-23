#ifndef CATCHCHALLENGER_SERVER_STRUCTURES_H
#define CATCHCHALLENGER_SERVER_STRUCTURES_H

#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QTimer>
#endif

#include <QObject>
#include <QList>
#include <QStringList>
#include <QString>
#include <QMutex>
#include <QHash>
#include <QVariant>
#include <QSet>
#include <QDataStream>
#include <QMultiHash>
#include <QRegularExpression>
#include <QDateTime>
#include <QProcess>

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
    QString skin;
};

struct FileToSend
{
    //not QFile * to prevent too many file open
    QString file;
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

struct PlayerOnMap
{
    CommonMap* map;
    COORD_TYPE x;
    COORD_TYPE y;
    Orientation orientation;
};

struct NormalServerSettings
{
    quint16 server_port;
    QString server_ip;
    QString proxy;
    quint16 proxy_port;
    bool useSsl;
    #ifdef Q_OS_LINUX
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
    bool sendPlayerNumber;
    bool anonymous;
    bool benchmark;
    //fight
    bool pvp;
    quint16 max_players;//not common because if null info not send

    //the listen, implicit on the client
    QString datapack_basePath;
    QString server_message;
    bool dontSendPlayerType;
    qint32 datapackCache;//-1 = disable, 0 = no timeout, else it's the timeout in s

    enum FightSync
    {
        FightSync_AtEachTurn=0x00,
        FightSync_AtTheEndOfBattle=0x01,//or at the object usage
        FightSync_AtTheDisconnexion=0x02
    };
    FightSync fightSync;
    bool positionTeleportSync;
    quint32 secondToPositionSync;//0 is disabled

    struct Database
    {
        QString file;
        QString host;
        QString db;
        QString login;
        QString pass;

        DatabaseBase::Type tryOpenType;
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
            bool reemit;
        };
        MapVisibility_Simple simple;
        struct MapVisibility_WithBorder
        {
            quint16 maxWithBorder;
            quint16 reshowWithBorder;
            quint16 max;
            quint16 reshow;
        };
        MapVisibility_WithBorder withBorder;
    };
    MapVisibility mapVisibility;

    //DDOS algorithm
    struct DDOS
    {
        #ifdef CATCHCHALLENGER_DDOS_FILTER
        quint8 kickLimitMove;
        quint8 kickLimitChat;
        quint8 kickLimitOther;
        #endif
        int computeAverageValueNumberOfValue;
        quint8 computeAverageValueTimeInterval;
        int dropGlobalChatMessageGeneral;
        int dropGlobalChatMessageLocalClan;
        int dropGlobalChatMessagePrivate;
    };
    DDOS ddos;

    //city
    City city;

    struct ProgrammedEvent
    {
        QString value;
        quint16 cycle;//mins
        quint16 offset;//mins
    };
    QHash<QString,QHash<QString,ProgrammedEvent> > programmedEventList;
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

struct Clan
{
    QString captureCityInProgress;
    QString capturedCity;
    quint32 clanId;
    QList<Client *> players;

    //the db info
    QString name;
    quint64 cash;
};

struct CaptureCityValidated
{
    QList<Client *> players;
    QList<Client *> playersInFight;
    QList<quint16> bots;
    QList<quint16> botsInFight;
    QHash<quint32,quint16> clanSize;
};

struct ServerProfileInternal
{
    MapServer *map;
    /*COORD_TYPE*/ quint8 x;
    /*COORD_TYPE*/ quint8 y;
    Orientation orientation;

    //only to add
    QStringList preparedQueryAdd;
    QStringList preparedQuerySelect;
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
    QList<TimerEvents *> timerEvents;
    #else
    QtDatabase *db_login;
    QtDatabase *db_base;
    QtDatabase *db_common;
    QtDatabase *db_server;
    QList<QtTimerEvents *> timerEvents;
    #endif

    QList<ServerProfileInternal> serverProfileInternalList;

    //datapack
    QString datapack_mapPath;
    QString mainDatapackFolder;
    QString subDatapackFolder;
    QRegularExpression datapack_rightFileName;
    QRegularExpression datapack_rightFolderName;

    //fight
    QMultiHash<quint16,MonsterDrops> monsterDrops;
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    std::vector<quint32> maxMonsterId;
    std::vector<quint32> maxClanId;
    #else
    std::atomic<unsigned int> maxClanId;
    std::atomic<unsigned int> maxMonsterId;
    #endif
    QHash<QString,QList<quint16> > captureFightIdList;
    QHash<QString,CityStatus> cityStatusList;
    QHash<quint32,QString> cityStatusListReverse;
    QSet<quint32> tradedMonster;
    QByteArray randomData;

    //market
    QList<MarketItem> marketItemList;
    QList<MarketPlayerMonster> marketPlayerMonsterList;

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
    quint32 maxAccountId;
    quint32 maxCharacterId;
    #endif
    QDateTime time_city_capture;
    QHash<quint32,Clan *> clanList;

    //map
    QHash<QString,CommonMap *> map_list;
    CommonMap ** flat_map_list;
    QHash<quint32,QString> id_map_to_map;
    qint8 sizeofInsertRequest;

    //connection
    quint16 connected_players;
    PlayerUpdater player_updater;
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    PlayerUpdaterToMaster player_updater_to_master;
    #endif
    QSet<quint32> connected_players_id_list;
    QStringList server_message;

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

    //datapack
    QHash<QString,quint8> skinList;
};

bool operator==(const CatchChallenger::MonsterDrops &monsterDrops1,const CatchChallenger::MonsterDrops &monsterDrops2);
bool operator<(const CatchChallenger::FileToSend &fileToSend1,const CatchChallenger::FileToSend &fileToSend2);
}

#endif // STRUCTURES_SERVER_H
