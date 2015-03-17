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
#include "../../general/base/CommonSettings.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"
#ifdef EPOLLCATCHCHALLENGERSERVER
#include "epoll/timer/TimerCityCapture.h"
#include "epoll/timer/TimerDdos.h"
#include "epoll/timer/TimerPositionSync.h"
#include "epoll/timer/TimerSendInsertMoveRemove.h"
#include "epoll/timer/TimerEvents.h"
#include "epoll/db/EpollPostgresql.h"
#else
#include "QtTimerEvents.h"
#include "QtDatabase.h"
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
    ServerPreparedDBQuery preparedDBQuery;

    struct Database
    {
        DatabaseBase::Type type;

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

struct TokenLink
{
    void * client;
    char value[CATCHCHALLENGER_TOKENSIZE];
};

struct ServerProfile
{
    QStringList preparedQuery;
    bool valid;
};

struct ServerPrivateVariables
{
    //bd
    #ifdef EPOLLCATCHCHALLENGERSERVER
    EpollPostgresql db;
    QList<TimerEvents *> timerEvents;
    #else
    QtDatabase db;
    QList<QtTimerEvents *> timerEvents;
    #endif

    QList<ServerProfile> serverProfileList;

    //datapack
    QString datapack_mapPath;
    QRegularExpression datapack_rightFileName;
    QRegularExpression datapack_rightFolderName;

    //fight
    QMultiHash<quint16,MonsterDrops> monsterDrops;
    quint32 maxMonsterId;
    QMutex monsterIdMutex;
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
    quint32 maxClanId;
    quint32 maxAccountId;
    quint32 maxCharacterId;
    QDateTime time_city_capture;
    QHash<quint32,Clan *> clanList;

    //map
    QHash<QString,CommonMap *> map_list;
    CommonMap ** flat_map_list;
    QHash<quint32,QString> id_map_to_map;
    qint8 sizeofInsertRequest;

    //connection
    #ifdef Q_OS_LINUX
    FILE *fpRandomFile;
    #endif
    quint16 connected_players;
    TokenLink tokenForAuth[CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION];
    quint32 tokenForAuthSize;
    PlayerUpdater player_updater;
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

    QList<ActionAllow> dictionary_allow;
    QList<quint8> dictionary_allow_reverse;
    QList<CommonMap *> dictionary_map;
    QList<int> dictionary_reputation;//negative == not found
    QList<quint8> dictionary_skin;
    QList<quint32> dictionary_skin_reverse;
    QHash<QString,QHash<QPair<quint8/*x*/,quint8/*y*/>,quint16/*db code*/> > dictionary_item;
    quint16 dictionary_item_maxId;
    QList<quint8> dictionary_item_reverse;
    //datapack
    QHash<QString,quint8> skinList;
};

bool operator==(const CatchChallenger::MonsterDrops &monsterDrops1,const CatchChallenger::MonsterDrops &monsterDrops2);
bool operator<(const CatchChallenger::FileToSend &fileToSend1,const CatchChallenger::FileToSend &fileToSend2);
}

#endif // STRUCTURES_SERVER_H
