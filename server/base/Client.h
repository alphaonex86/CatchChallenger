#ifndef CATCHCHALLENGER_CLIENT_H
#define CATCHCHALLENGER_CLIENT_H

#include <QObject>
#include <QByteArray>
#include <QTimer>
#include <QHash>

#include "../../general/base/DebugClass.h"
#include "ServerStructures.h"
#include "ClientMapManagement/ClientMapManagement.h"
#include "BroadCastWithoutSender.h"
#include "../../general/base/GeneralStructures.h"
#include "../../general/fight/CommonFightEngine.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../general/base/ProtocolParsing.h"
#include "../VariableServer.h"
#ifdef EPOLLCATCHCHALLENGERSERVER
#include "../epoll/BaseClassSwitch.h"
#else
#include <QObject>
#endif

namespace CatchChallenger {
class Client :
        #ifdef EPOLLCATCHCHALLENGERSERVER
        public BaseClassSwitch,
        #else
        public QObject,
        #endif
        public ProtocolParsingInputOutput, public CommonFightEngine, public ClientMapManagement
{
#ifndef EPOLLCATCHCHALLENGERSERVER
    Q_OBJECT
#endif
public:
    friend class BaseServer;
    explicit Client(
        #ifdef EPOLLCATCHCHALLENGERSERVER
            #ifdef SERVERSSL
                const int &infd, SSL_CTX *ctx
            #else
                const int &infd
            #endif
        #else
        ConnectedSocket *socket
        #endif
        );
    virtual ~Client();
    #ifdef EPOLLCATCHCHALLENGERSERVER
    BaseClassSwitch::Type getType() const;
    #endif
    //to get some info
    QString getPseudo();
    void savePosition();
    static bool characterConnected(const quint32 &characterId);
    void disconnectClient();
    static void disconnectClientById(const quint32 &characterId);
    Client *getClientFight();
    void doDDOSCompute();
    void receive_instant_player_number(const quint16 &connected_players, const char * const data, const quint8 &size);
    QByteArray getRawPseudo() const;
    void parseIncommingData();
    static void startTheCityCapture();
    static void setEvent(const quint8 &event, const quint8 &new_value);
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    static char * addAuthGetToken(const quint32 &characterId,const quint32 &accountIdRequester);
    #endif
    quint32 getPlayerId() const;
    quint32 getClanId() const;
    bool haveAClan() const;

    void sendFullPacket(const quint8 &mainIdent,const quint8 &subIdent,const char * const data,const unsigned int &size);
    void sendPacket(const quint8 &mainIdent,const char * const data,const unsigned int &size);
    void sendFullPacket(const quint8 &mainIdent,const quint8 &subIdent);
    void sendPacket(const quint8 &mainIdent);
    void sendRawSmallPacket(const char * const data,const unsigned int &size);

    static QList<int> generalChatDrop;
    static int generalChatDropTotalCache;
    static int generalChatDropNewValue;
    static QList<int> clanChatDrop;
    static int clanChatDropTotalCache;
    static int clanChatDropNewValue;
    static QList<int> privateChatDrop;
    static int privateChatDropTotalCache;
    static int privateChatDropNewValue;
    static QList<quint16> marketObjectIdList;
    static quint64 datapack_list_cache_timestamp;
    static QList<quint16> simplifiedIdList;
    static QHash<QString,Client *> playerByPseudo;
    static QHash<quint32,Clan *> clanList;
    static QList<Client *> clientBroadCastList;

    static unsigned char protocolReplyProtocolNotSupported[4];
    static unsigned char protocolReplyServerFull[4];
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    static unsigned char protocolReplyCompressionNone[4+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    static unsigned char protocolReplyCompresssionZlib[4+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    static unsigned char protocolReplyCompressionXz[4+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    #else
    static unsigned char protocolReplyCompressionNone[4];
    static unsigned char protocolReplyCompresssionZlib[4];
    static unsigned char protocolReplyCompressionXz[4];
    #endif

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    static unsigned char loginIsWrongBuffer[4];
    #endif

    static const unsigned char protocolHeaderToMatch[5];
protected:
    QByteArray rawPseudo;
    bool character_loaded,character_loaded_in_progress;
    QList<CatchChallenger::DatabaseBase::CallBack *> callbackRegistred;

    struct ClanActionParam
    {
        quint8 query_id;
        quint8 action;
        QString text;
    };
    struct AddCharacterParam
    {
        quint8 query_id;
        quint8 profileIndex;
        QString pseudo;
        quint8 skinId;
    };
    struct RemoveCharacterParam
    {
        quint8 query_id;
        quint32 characterId;
    };
    struct DeleteCharacterNow
    {
        quint32 characterId;
    };
    struct AskLoginParam
    {
        quint8 query_id;
        QByteArray login;
        QByteArray pass;
        QByteArray tempOutputData;
    };
    struct SelectCharacterParam
    {
        quint8 query_id;
        quint32 characterId;
    };
    struct SelectIndexParam
    {
        quint32 index;
    };
private:
    //-------------------
    quint32 account_id;//0 if not logged
    quint32 character_id;
    quint64 market_cash;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    bool isConnected;
    #endif
    quint16 randomIndex,randomSize;
    quint8 number_of_character;

    PlayerOnMap map_entry;
    PlayerOnMap rescue;
    PlayerOnMap unvalidated_rescue;
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

    qint32 connected_players;//it's th last number of connected player send
    QList<void *> paramToPassToCallBack;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    QStringList paramToPassToCallBackType;
    #endif
    static QList<quint8> selectCharacterQueryId;

    // for status
    bool have_send_protocol;
    bool is_logging_in_progess;
    bool stopIt;
    quint8 movePacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE];
    quint8 movePacketKickSize;
    quint8 movePacketKickTotalCache;
    quint8 movePacketKickNewValue;
    quint8 chatPacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE];
    quint8 chatPacketKickSize;
    quint8 chatPacketKickTotalCache;
    quint8 chatPacketKickNewValue;
    quint8 otherPacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE];
    quint8 otherPacketKickSize;
    quint8 otherPacketKickTotalCache;
    quint8 otherPacketKickNewValue;
    quint8 profileIndex;
    QList<PlayerOnMap> lastTeleportation;
    QList<quint8> queryNumberList;

    Client *otherPlayerBattle;
    bool battleIsValidated;
    quint32 mCurrentSkillId;
    bool mHaveCurrentSkill,mMonsterChange;
    quint32 botFightCash;
    quint32 botFightId;
    bool isInCityCapture;
    QList<Skill::AttackReturn> attackReturn;
    QHash<quint32, QHash<quint32,quint32> > deferedEndurance;
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    struct TokenAuth
    {
        char *token;
        quint32 characterId;
        quint32 accountIdRequester;
        quint32 createTime;
    };
    static std::vector<TokenAuth> tokenAuthList;
    #endif

    //player indexed list
    static const QString text_chat;
    static const QString text_space;
    static const QString text_system;
    static const QString text_system_important;
    static const QString text_setrights;
    static const QString text_normal;
    static const QString text_premium;
    static const QString text_gm;
    static const QString text_dev;
    static const QString text_playerlist;
    static const QString text_startbold;
    static const QString text_stopbold;
    static const QString text_playernumber;
    static const QString text_kick;
    static const QString text_Youarealoneontheserver;
    static const QString text_playersconnected;
    static const QString text_playersconnectedspace;
    static const QString text_havebeenkickedby;
    static const QString text_unknowcommand;
    static const QString text_commandnotunderstand;
    static const QString text_command;
    static const QString text_commaspace;
    static const QString text_unabletofoundtheconnectedplayertokick;
    static const QString text_unabletofoundthisrightslevel;

    static const QRegularExpression commandRegExp;
    static const QRegularExpression commandRegExpWithArgs;
    static const QRegularExpression isolateTheMainCommand;
    static const QString text_server_full;
    static const QString text_slashpmspace;
    static const QString text_slash;
    static const QString text_regexresult1;
    static const QString text_regexresult2;
    static const QString text_send_command_slash;
    static const QString text_trade;
    static const QString text_battle;
    static const QString text_give;
    static const QString text_setevent;
    static const QString text_take;
    static const QString text_tp;
    static const QString text_stop;
    static const QString text_restart;
    static const QString text_unknown_send_command_slash;
    static const QString text_commands_seem_not_right;
    static quint8 tempDatapackListReplySize;
    static QByteArray tempDatapackListReplyArray;
    static quint8 tempDatapackListReply;
    static int tempDatapackListReplyTestCount;
    struct DatapackCacheFile
    {
        quint32 mtime;
        quint32 partialHash;
    };
    static QHash<QString,quint32> datapack_file_list_cache;
    static QHash<QString,DatapackCacheFile> datapack_file_hash_cache;
    static QRegularExpression fileNameStartStringRegex;
    static QString single_quote;
    static QString antislash_single_quote;
    static const QString text_dotslash;
    static const QString text_dotcomma;
    static const QString text_double_slash;
    static const QString text_antislash;
    static const QString text_top;
    static const QString text_bottom;
    static const QString text_left;
    static const QString text_right;
    static const QString text_dottmx;
    static const QString text_unknown;
    static const QString text_female;
    static const QString text_male;
    static const QString text_warehouse;
    static const QString text_wear;
    static const QString text_market;

    //info linked
    static Direction	temp_direction;
    static QHash<quint32,Client *> playerById;
    static QHash<QString,QList<Client *> > captureCity;
    static QHash<QString,CaptureCityValidated> captureCityValidatedList;

    static const QString text_0;
    static const QString text_1;
    static const QString text_false;
    static const QString text_true;
    static const QString text_to;

    //socket related
    #ifndef EPOLLCATCHCHALLENGERSERVER
    void connectionError(QAbstractSocket::SocketError);
    #endif

    #ifndef EPOLLCATCHCHALLENGERSERVER
    /// \warning it need be complete protocol trame
    void fake_receive_data(QByteArray data);
    #endif
    //global slot
    void sendPM(const QString &text,const QString &pseudo);
    void receiveChatText(const Chat_type &chatType, const QString &text, const Client *sender_informations);
    void receiveSystemText(const QString &text,const bool &important=false);
    void sendChatText(const Chat_type &chatType,const QString &text);
    void sendBroadCastCommand(const QString &command,const QString &extraText);
    void setRights(const Player_type& type);
    //normal slots
    void sendSystemMessage(const QString &text,const bool &important=false);
    //clan
    void clanChangeWithoutDb(const quint32 &clanId);

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    void askLogin(const quint8 &query_id, const char *rawdata);
    void createAccount(const quint8 &query_id, const char *rawdata);
    static void createAccount_static(void *object);
    void createAccount_object();
    void createAccount_return(AskLoginParam *askLoginParam);
    static void askLogin_static(void *object);
    void askLogin_object();
    void askLogin_return(AskLoginParam *askLoginParam);
    static void character_list_static(void *object);
    void character_list_object();
    QByteArray character_list_return(const quint8 &query_id);
    bool server_list();
    static void server_list_static(void *object);
    void server_list_object();
    void server_list_return(const quint8 &query_id,const QByteArray &previousData);
    void deleteCharacterNow(const quint32 &characterId);
    static void deleteCharacterNow_static(void *object);
    void deleteCharacterNow_object();
    void deleteCharacterNow_return(const quint32 &characterId);
    #endif
    //check each element of the datapack, determine if need be removed, updated, add as new file all the missing file
    void datapackList(const quint8 &query_id, const QStringList &files, const QList<quint32> &partialHashList);
    static QHash<QString,quint32> datapack_file_list(const bool withHash=true);
    QHash<QString,quint32> datapack_file_list_cached();
    void addDatapackListReply(const bool &fileRemove);
    void purgeDatapackListReply(const quint8 &query_id);
    void sendFileContent();
    void sendCompressedFileContent();
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    void dbQueryWriteLogin(const QString &queryText);
    #endif
    void dbQueryWriteCommon(const QString &queryText);
    void dbQueryWriteServer(const QString &queryText);
    //character
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    void addCharacter(const quint8 &query_id, const quint8 &profileIndex, const QString &pseudo, const quint8 &skinId);
    static void addCharacter_static(void *object);
    void addCharacter_object();
    void addCharacter_return(const quint8 &query_id, const quint8 &profileIndex, const QString &pseudo, const quint8 &skinId);
    void removeCharacterLater(const quint8 &query_id, const quint32 &characterId);
    static void removeCharacterLater_static(void *object);
    void removeCharacterLater_object();
    void removeCharacterLater_return(const quint8 &query_id, const quint32 &characterId);
    #endif
    void selectCharacter(const quint8 &query_id, const quint32 &characterId);
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    void selectCharacter(const quint8 &query_id, const char * const token);
    #endif
    static void selectCharacter_static(void *object);
    void selectCharacter_object();
    void selectCharacter_return(const quint8 &query_id, const quint32 &characterId);
    void selectCharacterServer(const quint8 &query_id, const quint32 &characterId);
    static void selectCharacterServer_static(void *object);
    void selectCharacterServer_object();
    void selectCharacterServer_return(const quint8 &query_id, const quint32 &characterId);

    static void selectClan_static(void *object);
    void selectClan_object();
    void selectClan_return();

    void fake_receive_data(const QByteArray &data);
    void purgeReadBuffer();

    void sendNewEvent(const QByteArray &data);
    void teleportTo(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    void sendTradeRequest(const QByteArray &data);
    void sendBattleRequest(const QByteArray &data);

    //chat
    void sendLocalChatText(const QString &text);
    //seed
    void seedValidated();
    void plantSeed(const quint8 &query_id,const quint8 &plant_id);
    void collectPlant(const quint8 &query_id);

    void createMemoryClan();
    Direction lookToMove(const Direction &direction);
    //seed
    void useSeed(const quint8 &plant_id);
    //crafting
    void useRecipe(const quint8 &query_id,const quint32 &recipe_id);
    void takeAnObjectOnMap();
    //inventory
    void addObjectAndSend(const quint16 &item,const quint32 &quantity=1);
    void addObject(const quint16 &item,const quint32 &quantity=1);
    void saveObjectRetention(const quint16 &item);
    quint32 removeObject(const quint16 &item,const quint32 &quantity=1);
    void sendRemoveObject(const quint16 &item,const quint32 &quantity=1);
    quint32 objectQuantity(const quint16 &item);
    bool addMarketCashWithoutSave(const quint64 &cash);
    void addCash(const quint64 &cash,const bool &forceSave=false);
    void removeCash(const quint64 &cash);
    void addWarehouseCash(const quint64 &cash,const bool &forceSave=false);
    void removeWarehouseCash(const quint64 &cash);
    void wareHouseStore(const qint64 &cash, const QList<QPair<quint16, qint32> > &items, const QList<quint32> &withdrawMonsters, const QList<quint32> &depositeMonsters);
    bool wareHouseStoreCheck(const qint64 &cash, const QList<QPair<quint16, qint32> > &items, const QList<quint32> &withdrawMonsters, const QList<quint32> &depositeMonsters);
    void addWarehouseObject(const quint16 &item,const quint32 &quantity=1);
    quint32 removeWarehouseObject(const quint16 &item,const quint32 &quantity=1);

    bool haveReputationRequirements(const QList<ReputationRequirements> &reputationList) const;
    void confirmEvolution(const quint32 &monsterId);
    void sendHandlerCommand(const QString &command,const QString &extraText);
    void addEventInQueue(const quint8 &event, const quint8 &event_value, const QDateTime &currentDateTime);
    void removeFirstEventInQueue();
    //inventory
    void destroyObject(const quint16 &itemId,const quint32 &quantity);
    void useObject(const quint8 &query_id,const quint16 &itemId);
    bool useObjectOnMonster(const quint16 &object, const quint32 &monster);
    //teleportation
    void receiveTeleportTo(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    //shop
    void getShopList(const quint8 &query_id, const quint16 &shopId);
    void buyObject(const quint8 &query_id, const quint16 &shopId, const quint16 &objectId, const quint32 &quantity, const quint32 &price);
    void sellObject(const quint8 &query_id,const quint16 &shopId,const quint16 &objectId,const quint32 &quantity,const quint32 &price);
    //factory
    void saveIndustryStatus(const quint32 &factoryId,const IndustryStatus &industryStatus,const Industry &industry);
    void getFactoryList(const quint8 &query_id, const quint16 &factoryId);
    void buyFactoryProduct(const quint8 &query_id,const quint16 &factoryId,const quint16 &objectId,const quint32 &quantity,const quint32 &price);
    void sellFactoryResource(const quint8 &query_id,const quint16 &factoryId,const quint16 &objectId,const quint32 &quantity,const quint32 &price);
    //trade
    void tradeCanceled();
    void tradeAccepted();
    void tradeFinished();
    void tradeAddTradeCash(const quint64 &cash);
    void tradeAddTradeObject(const quint16 &item,const quint32 &quantity);
    void tradeAddTradeMonster(const quint32 &monsterId);
    //quest
    void newQuestAction(const QuestAction &action,const quint32 &questId);
    void appendAllow(const ActionAllow &allow);
    void removeAllow(const ActionAllow &allow);
    //reputation
    void appendReputationPoint(const quint8 &reputationId, const qint32 &point);
    void appendReputationRewards(const QList<ReputationRewards> &reputationList);
    //battle
    void battleCanceled();
    void battleAccepted();
    virtual bool tryEscape();
    void heal();
    void requestFight(const quint16 &fightId);
    bool learnSkill(const quint32 &monsterId,const quint16 &skill);
    Client * getLocalClientHandlerFight();
    //clan
    void clanAction(const quint8 &query_id,const quint8 &action,const QString &text);
    void addClan_return(const quint8 &query_id, const quint8 &action, const QString &text);
    void addClan_object();
    static void addClan_static(void *object);
    void haveClanInfo(const quint32 &clanId, const QString &clanName, const quint64 &cash);
    void sendClanInfo();
    void clanInvite(const bool &accept);
    void waitingForCityCaputre(const bool &cancel);
    quint32 clanId() const;
    void previousCityCaptureNotFinished();
    void leaveTheCityCapture();
    void removeFromClan();
    void cityCaptureBattle(const quint16 &number_of_player,const quint16 &number_of_clan);
    void cityCaptureBotFight(const quint16 &number_of_player,const quint16 &number_of_clan,const quint32 &fightId);
    void cityCaptureInWait(const quint16 &number_of_player,const quint16 &number_of_clan);
    void cityCaptureWin();
    void fightOrBattleFinish(const bool &win,const quint32 &fightId);//fightId == 0 if is in battle
    static void cityCaptureSendInWait(const CaptureCityValidated &captureCityValidated,const quint16 &number_of_player,const quint16 &number_of_clan);
    static quint16 cityCapturePlayerCount(const CaptureCityValidated &captureCityValidated);
    static quint16 cityCaptureClanCount(const CaptureCityValidated &captureCityValidated);
    void moveMonster(const bool &up,const quint8 &number);
    //market
    void getMarketList(const quint32 &query_id);
    void buyMarketObject(const quint32 &query_id,const quint32 &marketObjectId,const quint32 &quantity);
    void buyMarketMonster(const quint32 &query_id,const quint32 &monsterId);
    void putMarketObject(const quint32 &query_id,const quint32 &objectId,const quint32 &quantity,const quint32 &price);
    void putMarketMonster(const quint32 &query_id, const quint32 &monsterId, const quint32 &price);
    void recoverMarketCash(const quint32 &query_id);
    void withdrawMarketObject(const quint32 &query_id,const quint32 &objectId,const quint32 &quantity);
    void withdrawMarketMonster(const quint32 &query_id, const quint32 &monsterId);

    static QString directionToStringToSave(const Direction &direction);
    static QString orientationToStringToSave(const Orientation &orientation);
    //quest
    bool haveNextStepQuestRequirements(const CatchChallenger::Quest &quest);
    bool haveStartQuestRequirement(const CatchChallenger::Quest &quest);
    bool nextStepQuest(const Quest &quest);
    bool startQuest(const Quest &quest);
    void addQuestStepDrop(const quint32 &questId,const quint8 &questStep);
    void removeQuestStepDrop(const quint32 &questId,const quint8 &questStep);

    bool checkCollision();

    bool getBattleIsValidated();
    bool isInFight() const;
    void saveCurrentMonsterStat();
    void saveMonsterStat(const PlayerMonster &monster);
    bool checkLoose();
    bool isInBattle() const;
    bool learnSkillInternal(const quint32 &monsterId,const quint32 &skill);
    void getRandomNumberIfNeeded() const;
    bool botFightCollision(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y);
    bool checkFightCollision(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y);
    void registerBattleRequest(Client * otherPlayerBattle);
    void saveAllMonsterPosition();

    void battleFinished();
    void battleFinishedReset();
    Client * getOtherPlayerBattle() const;
    bool finishTheTurn(const bool &isBot);
    bool useSkill(const quint32 &skill);
    bool currentMonsterAttackFirst(const PlayerMonster * currentMonster,const PublicPlayerMonster * otherMonster) const;
    void healAllMonsters();
    void battleFakeAccepted(Client * otherPlayer);
    void battleFakeAcceptedInternal(Client *otherPlayer);
    bool botFightStart(const quint32 &botFightId);
    void setInCityCapture(const bool &isInCityCapture);
    int addCurrentBuffEffect(const Skill::BuffEffect &effect);
    bool moveUpMonster(const quint8 &number);
    bool moveDownMonster(const quint8 &number);
    void saveMonsterPosition(const quint32 &monsterId,const quint8 &monsterPosition);
    bool doTheOtherMonsterTurn();
    Skill::AttackReturn generateOtherAttack();
    Skill::AttackReturn doTheCurrentMonsterAttack(const quint32 &skill, const quint8 &skillLevel);
    quint8 decreaseSkillEndurance(const quint32 &skill);
    void emitBattleWin();
    void hpChange(PlayerMonster * currentMonster, const quint32 &newHpValue);
    bool removeBuffOnMonster(PlayerMonster * currentMonster, const quint32 &buffId);
    bool removeAllBuffOnMonster(PlayerMonster * currentMonster);
    bool addLevel(PlayerMonster * monster, const quint8 &numberOfLevel=1);

    bool checkKOCurrentMonsters();
    void syncForEndOfTurn();
    void saveStat();
    bool buffIsValid(const Skill::BuffEffect &buffEffect);
    bool haveBattleAction() const;
    void resetBattleAction();
    quint8 getOtherSelectedMonsterNumber() const;
    void haveUsedTheBattleAction();
    void sendBattleReturn();
    void sendBattleMonsterChange();
    inline quint8 selectedMonsterNumberToMonsterPlace(const quint8 &selectedMonsterNumber);
    void internalBattleCanceled(const bool &send);
    void internalBattleAccepted(const bool &send);
    void resetTheBattle();
    PublicPlayerMonster *getOtherMonster();
    void fightFinished();
    bool giveXPSP(int xp,int sp);
    bool useSkillAgainstBotMonster(const quint32 &skill, const quint8 &skillLevel);
    void wildDrop(const quint32 &monster);
    quint8 getOneSeed(const quint8 &max);
    bool bothRealPlayerIsReady() const;
    bool checkIfCanDoTheTurn();
    bool dropKOOtherMonster();
    quint32 catchAWild(const bool &toStorage, const PlayerMonster &newMonster);
    bool haveCurrentSkill() const;
    quint32 getCurrentSkill() const;
    bool haveMonsterChange() const;
    bool addSkill(PlayerMonster * currentMonster,const PlayerMonster::PlayerSkill &skill);
    bool setSkillLevel(PlayerMonster * currentMonster,const int &index,const quint8 &level);
    bool removeSkill(PlayerMonster * currentMonster,const int &index);

    //trade
    Client * otherPlayerTrade;
    bool tradeIsValidated;
    bool tradeIsFreezed;
    quint64 tradeCash;
    QHash<quint32,quint32> tradeObjects;
    QList<PlayerMonster> tradeMonster;
    QList<quint32> inviteToClanList;
    Clan *clan;
public:
    #ifdef EPOLLCATCHCHALLENGERSERVER
    char *socketString;
    int socketStringSize;
    #endif

private:
    //city
    Client * otherCityPlayerBattle;

    //map move
    bool captureCityInProgress();
    //trade
    void internalTradeCanceled(const bool &send);
    void internalTradeAccepted(const bool &send);
    //other
    static MonsterDrops questItemMonsterToMonsterDrops(const Quest::ItemMonster &questItemMonster);
    bool otherPlayerIsInRange(Client * otherPlayer);

    static quint32 getMonsterId(bool * const ok);
    static quint32 getClanId(bool * const ok);
    bool getInTrade();
    void registerTradeRequest(Client * otherPlayerTrade);
    bool getIsFreezed();
    quint64 getTradeCash();
    QHash<quint32,quint32> getTradeObjects();
    QList<PlayerMonster> getTradeMonster();
    void resetTheTrade();
    void addExistingMonster(QList<PlayerMonster> tradeMonster);
    PlayerMonster &getSelectedMonster();
    quint8 getSelectedMonsterNumber();
    PlayerMonster& getEnemyMonster();
    void dissolvedClan();
    bool inviteToClan(const quint32 &clanId);
    void insertIntoAClan(const quint32 &clanId);
    void ejectToClan();

    void sendQuery(const quint8 &mainIdent,const quint8 &subIdent,const quint8 &queryNumber,const char * const data,const unsigned int &size);
    void sendQuery(const quint8 &mainIdent,const quint8 &subIdent,const quint8 &queryNumber);
    void postReply(const quint8 &queryNumber,const char * const data,const unsigned int &size);
    void postReply(const quint8 &queryNumber);

    void insertClientOnMap(CommonMap *map);
    void removeClientOnMap(CommonMap *map,const bool &withDestroy=false);
    void sendNearPlant();
    void removeNearPlant();

    void errorFightEngine(const QString &error);
    void messageFightEngine(const QString &message) const;

    struct PlantInWaiting
    {
        quint8 query_id;
        quint8 plant_id;
        CommonMap *map;
        quint8 x,y;
    };
    QList<PlantInWaiting> plant_list_in_waiting;

    void parseInputBeforeLogin(const quint8 &mainCodeType, const quint8 &queryNumber, const char * const data,const unsigned int &size);
    //have message without reply
    void parseMessage(const quint8 &mainCodeType,const char * const data,const unsigned int &size);
    void parseFullMessage(const quint8 &mainCodeType, const quint8 &subCodeType, const char * const rawData, const unsigned int &size);
    //have query with reply
    void parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size);
    void parseFullQuery(const quint8 &mainCodeType, const quint8 &subCodeType, const quint8 &queryNumber, const char * const rawData, const unsigned int &size);
    //send reply
    void parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size);
    void parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size);

    void parseNetworkReadError(const QString &errorString);

    // ------------------------------
    bool sendFile(const QString &fileName);

    void characterIsRight(const quint8 &query_id, quint32 characterId, CommonMap* map, const /*COORD_TYPE*/ quint8 &x, const /*COORD_TYPE*/ quint8 &y, const Orientation &orientation);
    void characterIsRightWithParsedRescue(const quint8 &query_id, quint32 characterId, CommonMap* map, const /*COORD_TYPE*/ quint8 &x, const /*COORD_TYPE*/ quint8 &y, const Orientation &orientation,
                      CommonMap* rescue_map, const /*COORD_TYPE*/ quint8 &rescue_x, const /*COORD_TYPE*/ quint8 &rescue_y, const Orientation &rescue_orientation,
                      CommonMap* unvalidated_rescue_map, const /*COORD_TYPE*/ quint8 &unvalidated_rescue_x, const /*COORD_TYPE*/ quint8 &unvalidated_rescue_y, const Orientation &unvalidated_rescue_orientation
                      );
    void characterIsRightWithRescue(const quint8 &query_id,quint32 characterId,CommonMap* map,const /*COORD_TYPE*/ quint8 &x,const /*COORD_TYPE*/ quint8 &y,const Orientation &orientation,
                      const QVariant &rescue_map,const QVariant &rescue_x,const QVariant &rescue_y,const QVariant &rescue_orientation,
                      const QVariant &unvalidated_rescue_map,const QVariant &unvalidated_rescue_x,const QVariant &unvalidated_rescue_y,const QVariant &unvalidated_rescue_orientation
                      );
    void characterIsRightFinalStep();
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    void loginIsWrong(const quint8 &query_id,const quint8 &returnCode,const QString &debugMessage);
    #endif
    void characterSelectionIsWrong(const quint8 &query_id,const quint8 &returnCode,const QString &debugMessage);
    //load linked data (like item, quests, ...)
    void loadLinkedData();
    void loadItems();
    void loadItemsWarehouse();
    void loadRecipes();
    void loadMonsters();
    void loadMonstersWarehouse();
    void loadReputation();
    void loadQuests();
    void loadBotAlreadyBeaten();
    void loadPlayerMonsterBuffs(const quint32 &index);
    void loadPlayerMonsterSkills(const quint32 &index);
    void loadPlayerAllow();

    static void loadItems_static(void *object);
    void loadItems_return();
    static void loadItemsWarehouse_static(void *object);
    void loadItemsWarehouse_return();
    static void loadRecipes_static(void *object);
    void loadRecipes_return();
    static void loadMonsters_static(void *object);
    void loadMonsters_return();
    static void loadMonstersWarehouse_static(void *object);
    void loadMonstersWarehouse_return();
    static void loadReputation_static(void *object);
    void loadReputation_return();
    static void loadQuests_static(void *object);
    void loadQuests_return();
    static void loadBotAlreadyBeaten_static(void *object);
    void loadBotAlreadyBeaten_return();
    static void loadPlayerAllow_static(void *object);
    void loadPlayerAllow_return();
    void loadItemOnMap();
    static void loadItemOnMap_static(void *object);
    void loadItemOnMap_return();

    static void loadPlayerMonsterBuffs_static(void *object);
    void loadPlayerMonsterBuffs_object();
    void loadPlayerMonsterBuffs_return(const quint32 &index);
    static void loadPlayerMonsterSkills_static(void *object);
    void loadPlayerMonsterSkills_object();
    void loadPlayerMonsterSkills_return(const quint32 &index);

    quint32 tryCapture(const quint16 &item);
    bool changeOfMonsterInFight(const quint32 &monsterId);
    void confirmEvolutionTo(PlayerMonster * playerMonster,const quint32 &monster);

    void sendInventory();

    void generateRandomNumber();
    quint32 randomSeedsSize() const;
protected:
    bool loadTheRawUTF8String();//virtual to load dynamic max to_send_move
    //normal management related
    void errorOutput(const QString &errorString);
    void kick();
    void normalOutput(const QString &message) const;
    //drop all clients
    void dropAllClients();
    void dropAllBorderClients();
    //input/ouput layer
    void errorParsingLayer(const QString &error);
    void messageParsingLayer(const QString &message) const;
    //map move
    virtual bool singleMove(const Direction &direction);
    virtual void put_on_the_map(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    virtual void teleportValidatedTo(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    virtual bool moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
    virtual void extraStop() = 0;
};
}

#endif // CLIENT_H
