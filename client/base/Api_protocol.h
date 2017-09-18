#ifndef CATCHCHALLENGER_PROTOCOL_H
#define CATCHCHALLENGER_PROTOCOL_H

#include <QObject>
#include <QString>
#include <QCoreApplication>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QCryptographicHash>
#include <vector>

#include "ClientStructures.h"
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/MoveOnTheMap.h"

namespace CatchChallenger {
class Api_protocol : public QObject, public ProtocolParsingInputOutput, public MoveOnTheMap
{
    Q_OBJECT
public:
    static bool internalVersionDisplayed;
    explicit Api_protocol(ConnectedSocket *socket,bool tolerantMode=false);
    ~Api_protocol();
    void disconnectClient();
    void unloadSelection();
    ServerFromPoolForDisplay getCurrentServer(const int &index);
    bool dataToPlayerMonster(QDataStream &in, PlayerMonster &monster);

    //protocol command
    bool tryLogin(const QString &login,const QString &pass);
    bool tryCreateAccount();
    bool sendProtocol();
    bool protocolWrong() const;
    virtual QString socketDisconnectedForReconnect();

    //get the stored data
    Player_private_and_public_informations &get_player_informations();
    const Player_private_and_public_informations &get_player_informations_ro() const;
    QString getPseudo();
    uint16_t getId();

    virtual void sendDatapackContentBase(const QByteArray &hashBase=QByteArray()) = 0;
    virtual void sendDatapackContentMainSub(const QByteArray &hashMain=QByteArray(),const QByteArray &hashSub=QByteArray()) = 0;
    virtual void tryDisconnect() = 0;
    virtual QString datapackPathBase() const;
    virtual QString datapackPathMain() const;
    virtual QString datapackPathSub() const;
    virtual QString mainDatapackCode() const;
    virtual QString subDatapackCode() const;
    void setDatapackPath(const QString &datapackPath);
    QByteArray toUTF8WithHeader(const QString &text);
    void have_main_and_sub_datapack_loaded();//can now load player_informations
    bool character_select_is_send();

    enum StageConnexion
    {
        Stage1=0x01,//Connect on login server
        Stage2=0x02,//reconnexion in progress
        Stage3=0x03,//connecting on game server
        Stage4=0x04,//connected on game server
    };
    StageConnexion stage() const;
    enum DatapackStatus
    {
        Base=0x01,
        Main=0x02,
        Sub=0x03,
        Finished=0x04
    };
    DatapackStatus datapackStatus;

    //to reset all
    void resetAll();

    bool getIsLogged() const;
    bool getCaracterSelected() const;
    QMap<uint8_t,QTime> getQuerySendTimeList() const;

    //to manipulate the monsters
    Player_private_and_public_informations player_informations;

    enum ProxyMode
    {
        Reconnect=0x01,
        Proxy=0x02
    };
    ProxyMode proxyMode;

    bool setMapNumber(const unsigned int number_of_map);
private:
    //status for the query
    bool haveFirstHeader;
    bool is_logged;
    bool character_selected;
    bool character_select_send;
    bool have_send_protocol;
    bool have_receive_protocol;
    bool tolerantMode;
    bool haveTheServerList;
    bool haveTheLogicalGroupList;

    static QString text_balise_root_start;
    static QString text_balise_root_stop;
    static QString text_name;
    static QString text_description;
    static QString text_en;
    static QString text_lang;
    static QString text_slash;

    LogicialGroup logicialGroup;

    StageConnexion stageConnexion;

    //to send trame
    std::vector<uint8_t> lastQueryNumber;

    #ifdef BENCHMARKMUTIPLECLIENT
    static char hurgeBufferForBenchmark[4096];
    static bool precomputeDone;
    static char hurgeBufferMove[4];
    #endif

    struct DelayedReply
    {
        uint8_t packetCode;
        uint8_t queryNumber;
        QByteArray data;
    };
    DelayedReply delayedLogin;
    struct DelayedMessage
    {
        uint8_t packetCode;
        QByteArray data;
    };
    std::vector<DelayedMessage> delayedMessages;
protected:
    virtual void socketDestroyed();
    void parseIncommingData();
    void readForFirstHeader();
    void sslHandcheckIsFinished();
    void connectTheExternalSocketInternal();
    void saveCert(const QString &file);

    void errorParsingLayer(const std::string &error);
    void messageParsingLayer(const std::string &message) const;

    bool parseCharacterBlockServer(const uint8_t &packetCode,const uint8_t &queryNumber,const QByteArray &data);
    bool parseCharacterBlockCharacter(const uint8_t &packetCode,const uint8_t &queryNumber,const QByteArray &data);

    /// \note This is note into server part to force to write manually the serial and improve the performance, this function is more easy but implies lot of memory copy via SIMD
    //send message without reply
    bool packOutcommingData(const uint8_t &packetCode,const char * const data,const int &size);
    //send query with reply
    bool packOutcommingQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const int &size);
    //send reply
    bool postReplyData(const uint8_t &queryNumber, const char * const data,const int &size);
    //the internal serialiser
    void send_player_move_internal(const uint8_t &moved_unit,const CatchChallenger::Direction &direction);
protected:
    //have message without reply
    virtual bool parseMessage(const uint8_t &packetCode,const char * const data,const unsigned int &size);
    //have query with reply
    virtual bool parseQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size);
    //send reply
    virtual bool parseReplyData(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size);

    //have message without reply
    virtual bool parseMessage(const uint8_t &packetCode,const QByteArray &data);
    //have query with reply
    virtual bool parseQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const QByteArray &data);
    //send reply
    virtual bool parseReplyData(const uint8_t &packetCode,const uint8_t &queryNumber,const QByteArray &data);

    //servers list
    LogicialGroup * addLogicalGroup(const QString &path, const QString &xml, const QString &language);
    ServerFromPoolForDisplay * addLogicalServer(const ServerFromPoolForDisplayTemp &server, const QString &language);

    //error
    void parseError(const QString &userMessage, const QString &errorString);

    //general data
    virtual void defineMaxPlayers(const uint16_t &maxPlayers) = 0;

    //stored local player info
    uint16_t max_players;
    uint16_t max_players_real;
    uint32_t number_of_map;

    //to send trame
    uint8_t queryNumber();
    static QSet<QString> extensionAllowed;
    QMap<uint8_t,QTime> querySendTime;

    //inventory
    QList<uint32_t> lastObjectUsed;

    //datapack
    QString mDatapackBase;
    QString mDatapackMain;
    QString mDatapackSub;

    //teleport list query id
    struct TeleportQueryInformation
    {
        uint8_t queryId;
        Direction direction;//for the internal set of last_direction
    };

    QList<TeleportQueryInformation> teleportList;

    //trade
    QList<uint32_t> tradeRequestId;
    bool isInTrade;
    //battle
    QList<uint32_t> battleRequestId;
    bool isInBattle;
    QByteArray token;
    QByteArray passHash;
    QByteArray loginHash;

    //server list
    int32_t selectedServerIndex;
    QList<ServerFromPoolForDisplay *> serverOrdenedList;
    QList<LogicialGroup *> logicialGroupIndexList;
    QList<QList<CharacterEntry> > characterListForSelection;
    QByteArray tokenForGameServer;
signals:
    void newError(const QString &error,const QString &detailedError) const;
    void message(const QString &message) const;
    void lastReplyTime(const uint32_t &time) const;

    //protocol/connection info
    void disconnected(const QString &reason) const;
    void notLogged(const QString &reason) const;
    void logged(const QList<ServerFromPoolForDisplay *> &serverOrdenedList,const QList<QList<CharacterEntry> > &characterEntryList) const;
    void protocol_is_good() const;
    void connectedOnLoginServer() const;
    void connectingOnGameServer() const;
    void connectedOnGameServer() const;
    void haveDatapackMainSubCode();
    void gatewayCacheUpdate(const uint8_t gateway,const uint8_t progression);

    //general info
    void number_of_player(const uint16_t &number,const uint16_t &max_players) const;
    void random_seeds(const QByteArray &data) const;

    //character
    void newCharacterId(const uint8_t &returnCode,const uint32_t &characterId) const;
    void haveCharacter() const;
    //events
    void setEvents(const QList<QPair<uint8_t,uint8_t> > &events) const;
    void newEvent(const uint8_t &event,const uint8_t &event_value) const;

    //map move
    void insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction) const;
    void move_player(const uint16_t &id,const QList<QPair<uint8_t,CatchChallenger::Direction> > &movement) const;
    void remove_player(const uint16_t &id) const;
    void reinsert_player(const uint16_t &id,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction) const;
    void full_reinsert_player(const uint16_t &id,const uint32_t &mapId,const uint8_t &x,const uint8_t y,const CatchChallenger::Direction &direction) const;
    void dropAllPlayerOnTheMap() const;
    void teleportTo(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction) const;

    //plant
    void insert_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const uint8_t &plant_id,const uint16_t &seconds_to_mature) const;
    void remove_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y) const;
    void seed_planted(const bool &ok) const;
    void plant_collected(const CatchChallenger::Plant_collect &stat) const;
    //crafting
    void recipeUsed(const RecipeUsage &recipeUsage) const;
    //inventory
    void objectUsed(const ObjectUsage &objectUsage) const;
    void monsterCatch(const bool &success) const;

    //chat
    void new_chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &player_type) const;
    void new_system_text(const CatchChallenger::Chat_type &chat_type,const QString &text) const;

    //player info
    void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations) const;
    void have_inventory(const std::unordered_map<uint16_t,uint32_t> &items,const std::unordered_map<uint16_t,uint32_t> &warehouse_items) const;
    void add_to_inventory(const QHash<uint16_t,uint32_t> &items) const;
    void remove_to_inventory(const QHash<uint16_t,uint32_t> &items) const;

    //datapack
    void haveTheDatapack() const;
    void haveTheDatapackMainSub() const;
    //base
    void newFileBase(const QString &fileName,const QByteArray &data) const;
    void newHttpFileBase(const QString &url,const QString &fileName) const;
    void removeFileBase(const QString &fileName) const;
    void datapackSizeBase(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize) const;
    //main
    void newFileMain(const QString &fileName,const QByteArray &data) const;
    void newHttpFileMain(const QString &url,const QString &fileName) const;
    void removeFileMain(const QString &fileName) const;
    void datapackSizeMain(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize) const;
    //sub
    void newFileSub(const QString &fileName,const QByteArray &data) const;
    void newHttpFileSub(const QString &url,const QString &fileName) const;
    void removeFileSub(const QString &fileName) const;
    void datapackSizeSub(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize) const;

    //shop
    void haveShopList(const QList<ItemToSellOrBuy> &items) const;
    void haveBuyObject(const BuyStat &stat,const uint32_t &newPrice) const;
    void haveSellObject(const SoldStat &stat,const uint32_t &newPrice) const;

    //factory
    void haveFactoryList(const uint32_t &remainingProductionTime,const QList<ItemToSellOrBuy> &resources,const QList<ItemToSellOrBuy> &products) const;
    void haveBuyFactoryObject(const BuyStat &stat,const uint32_t &newPrice) const;
    void haveSellFactoryObject(const SoldStat &stat,const uint32_t &newPrice) const;

    //trade
    void tradeRequested(const QString &pseudo,const uint8_t &skinInt) const;
    void tradeAcceptedByOther(const QString &pseudo,const uint8_t &skinInt) const;
    void tradeCanceledByOther() const;
    void tradeFinishedByOther() const;
    void tradeValidatedByTheServer() const;
    void tradeAddTradeCash(const quint64 &cash) const;
    void tradeAddTradeObject(const uint32_t &item,const uint32_t &quantity) const;
    void tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster) const;

    //battle
    void battleRequested(const QString &pseudo,const uint8_t &skinInt) const;
    void battleAcceptedByOther(const QString &pseudo,const uint8_t &skinId,const QList<uint8_t> &stat,const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster) const;
    void battleCanceledByOther() const;
    void sendBattleReturn(const QList<Skill::AttackReturn> &attackReturn) const;

    //clan
    void clanActionSuccess(const uint32_t &clanId) const;
    void clanActionFailed() const;
    void clanDissolved() const;
    void clanInformations(const QString &name) const;
    void clanInvite(const uint32_t &clanId,const QString &name) const;
    void cityCapture(const uint32_t &remainingTime,const uint8_t &type) const;

    //city
    void captureCityYourAreNotLeader();
    void captureCityYourLeaderHaveStartInOtherCity(const QString &zone);
    void captureCityPreviousNotFinished();
    void captureCityStartBattle(const uint16_t &player_count,const uint16_t &clan_count);
    void captureCityStartBotFight(const uint16_t &player_count,const uint16_t &clan_count,const uint32_t &fightId);
    void captureCityDelayedStart(const uint16_t &player_count,const uint16_t &clan_count);
    void captureCityWin();

    //market
    void marketList(const quint64 &price,const QList<MarketObject> &marketObjectList,const QList<MarketMonster> &marketMonsterList,const QList<MarketObject> &marketOwnObjectList,const QList<MarketMonster> &marketOwnMonsterList) const;
    void marketBuy(const bool &success) const;
    void marketBuyMonster(const PlayerMonster &playerMonster) const;
    void marketPut(const bool &success) const;
    void marketGetCash(const quint64 &cash) const;
    void marketWithdrawCanceled() const;
    void marketWithdrawObject(const uint32_t &objectId,const uint32_t &quantity) const;
    void marketWithdrawMonster(const PlayerMonster &playerMonster) const;
public:
    void send_player_direction(const CatchChallenger::Direction &the_direction);
    void send_player_move(const uint8_t &moved_unit,const CatchChallenger::Direction &direction);
    //void send_move_unit(const CatchChallenger::Direction &direction);->do by MoveOnTheMap::newDirection()
    void sendChatText(const CatchChallenger::Chat_type &chatType,const QString &text);
    void sendPM(const QString &text,const QString &pseudo);
    bool teleportDone();

    //character
    bool addCharacter(const uint8_t &charactersGroupIndex,const uint8_t &profileIndex, const QString &pseudo, const uint8_t &monsterGroupId, const uint8_t &skinId);
    bool removeCharacter(const uint8_t &charactersGroupIndex,const uint32_t &characterId);
    bool selectCharacter(const uint8_t &charactersGroupIndex, const uint32_t &serverUniqueKey, const uint32_t &characterId);
    bool selectCharacter(const uint8_t &charactersGroupIndex, const uint32_t &serverUniqueKey, const uint32_t &characterId,const uint32_t &serverIndex);
    LogicialGroup getLogicialGroup() const;

    //plant, can do action only if the previous is finish
    void useSeed(const uint8_t &plant_id);
    void collectMaturePlant();
    //crafting
    void useRecipe(const uint16_t &recipeId);
    void addRecipe(const uint16_t &recipeId);

    //trade
    void tradeRefused();
    void tradeAccepted();
    void tradeCanceled();
    void tradeFinish();
    void addTradeCash(const quint64 &cash);
    void addObject(const uint16_t &item,const uint32_t &quantity);
    void addMonsterByPosition(const uint8_t &monsterPosition);

    //battle
    void battleRefused();
    void battleAccepted();

    //inventory
    void destroyObject(const uint16_t &object,const uint32_t &quantity=1);
    bool useObject(const uint16_t &object);
    bool useObjectOnMonsterByPosition(const uint16_t &object, const uint8_t &monsterPosition);
    void wareHouseStore(const qint64 &cash, const QList<QPair<uint16_t, int32_t> > &items, const QList<uint32_t> &withdrawMonsters, const QList<uint32_t> &depositeMonsters);
    void takeAnObjectOnMap();

    //shop
    void getShopList(const uint32_t &shopId);
    void buyObject(const uint32_t &shopId,const uint32_t &objectId,const uint32_t &quantity,const uint32_t &price);
    void sellObject(const uint32_t &shopId,const uint32_t &objectId,const uint32_t &quantity,const uint32_t &price);

    //factory
    void getFactoryList(const uint16_t &factoryId);
    void buyFactoryProduct(const uint16_t &factoryId,const uint16_t &objectId,const uint32_t &quantity,const uint32_t &price);
    void sellFactoryResource(const uint16_t &factoryId, const uint16_t &objectId, const uint32_t &quantity, const uint32_t &price);

    //fight
    void tryEscape();
    void useSkill(const uint16_t &skill);
    void heal();
    void requestFight(const uint32_t &fightId);
    void changeOfMonsterInFightByPosition(const uint8_t &monsterPosition);

    //monster
    void learnSkillByPosition(const uint8_t &monsterPosition, const uint16_t &skill);
    void monsterMoveDown(const uint8_t &number);
    void monsterMoveUp(const uint8_t &number);
    void confirmEvolutionByPosition(const uint8_t &monterPosition);

    //quest
    void startQuest(const uint16_t &questId);
    void finishQuest(const uint16_t &questId);
    void cancelQuest(const uint16_t &questId);
    void nextQuestStep(const uint16_t &questId);

    //clan
    void createClan(const QString &name);
    void leaveClan();
    void dissolveClan();
    void inviteClan(const QString &pseudo);
    void ejectClan(const QString &pseudo);
    void inviteAccept(const bool &accept);
    void waitingForCityCapture(const bool &cancel);

    //market
    void getMarketList();
    void buyMarketObject(const uint32_t &marketObjectId,const uint32_t &quantity=1);
    void buyMarketMonster(const uint32_t &monsterMarketId);
    void putMarketObject(const uint16_t &objectId, const uint32_t &quantity, const uint64_t &price);
    void putMarketMonsterByPosition(const uint8_t &monsterPosition,const uint64_t &price);
    void recoverMarketCash();
    void withdrawMarketObject(const uint16_t &objectPosition, const uint32_t &quantity=1);
    void withdrawMarketMonster(const uint32_t &monsterMarketId);
};
}

#endif // CATCHCHALLENGER_PROTOCOL_H
