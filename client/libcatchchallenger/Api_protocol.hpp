#ifndef CATCHCHALLENGER_PROTOCOL_H
#define CATCHCHALLENGER_PROTOCOL_H
#if ! defined (ONLYMAPRENDER)

#include <vector>
#include <map>

#include "ClientStructures.hpp"
#include "../../general/base/GeneralStructures.hpp"
#if ! defined (ONLYMAPRENDER)
#include "../../general/base/ProtocolParsing.hpp"
#endif
#include "../../general/base/MoveOnTheMap.hpp"
#include "../../general/base/lib.h"

namespace CatchChallenger {
class DLL_PUBLIC Api_protocol : public ProtocolParsingInputOutput, public MoveOnTheMap
{
public:
    explicit Api_protocol();
    ~Api_protocol();
    void unloadSelection();
    const ServerFromPoolForDisplay &getCurrentServer(const unsigned int &index);
    struct ChatEntry
    {
        std::string player_pseudo;
        Player_type player_type;
        Chat_type chat_type;
        std::string text;
    };
    const std::vector<ChatEntry> &getChatContent();
    void add_system_text(Chat_type chat_type,std::string text);
    void add_chat_text(Chat_type chat_type, std::string text, std::string pseudo, CatchChallenger::Player_type player_type);
    //reputation
    bool haveReputationRequirements(const std::vector<CatchChallenger::ReputationRequirements> &reputationList) const;
    //quest
    bool haveNextStepQuestRequirements(const CatchChallenger::Quest &quest) const;
    bool haveStartQuestRequirement(const CatchChallenger::Quest &quest) const;
    //item
    uint32_t itemQuantity(const uint16_t &itemId) const;
    //fight
    void addBeatenBotFight(const uint16_t &botFightId);
    virtual bool haveBeatBot(const uint16_t &botFightId) const = 0;

    //protocol command
    void hashSha224(const char * const data,const int size,char *buffer);
    bool tryLogin(const std::string &login,const std::string &pass);
    bool tryCreateAccount();
    bool sendProtocol();
    bool protocolWrong() const;
    virtual std::string socketDisconnectedForReconnect();
    const std::vector<ServerFromPoolForDisplay> &getServerOrdenedList();
    int dataToPlayerMonster(const char * const data, const unsigned int &size, PlayerMonster &monster);
    //virtual void disconnectFromHost();

    //get the stored data
    Player_private_and_public_informations &get_player_informations();
    const Player_private_and_public_informations &get_player_informations_ro() const;
    std::string getPseudo() const;
    uint16_t getId() const;

    virtual void sendDatapackContentBase(const std::string &hashBase=std::string()) = 0;
    virtual void sendDatapackContentMainSub(const std::string &hashMain=std::string(),const std::string &hashSub=std::string()) = 0;
    virtual void tryDisconnect() = 0;
    virtual std::string datapackPathBase() const;
    virtual std::string datapackPathMain() const;
    virtual std::string datapackPathSub() const;
    virtual std::string mainDatapackCode() const;
    virtual std::string subDatapackCode() const;
    void setDatapackPath(const std::string &datapackPath);
    std::string toUTF8WithHeader(const std::string &text);
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
    std::map<uint8_t,uint64_t> getQuerySendTimeList() const;
    std::vector<uint8_t> &getEvents();

    //to manipulate the monsters
    Player_private_and_public_informations player_informations;
    std::vector<uint8_t> events;

    enum ProxyMode
    {
        Reconnect=0x01,
        Proxy=0x02
    };
    ProxyMode proxyMode;

    bool setMapNumber(const unsigned int number_of_map);
    virtual bool disconnectClient() override;
    std::pair<uint16_t,uint16_t> getLast_number_of_player();
protected:
    std::vector<ChatEntry> chat_list;

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
    uint16_t last_players_number;
    uint16_t last_max_players;

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
        std::string data;
    };
    DelayedReply delayedLogin;
    struct DelayedMessage
    {
        uint8_t packetCode;
        std::string data;
    };
    std::vector<DelayedMessage> delayedMessages;
protected:
    virtual void socketDestroyed();
    void parseIncommingData() override;
    virtual void readForFirstHeader() = 0;
    void sslHandcheckIsFinished();
    void connectTheExternalSocketInternal();
    #ifndef __EMSCRIPTEN__
    void saveCert(const std::string &file);
    #endif

    void errorParsingLayer(const std::string &error) override;
    void messageParsingLayer(const std::string &message) const override;

    bool parseCharacterBlockServer(const uint8_t &packetCode, const uint8_t &queryNumber, const char * const data, const int &size);
    bool parseCharacterBlockCharacter(const uint8_t &packetCode, const uint8_t &queryNumber, const char * const data, const int &size);

    /// \note This is note into server part to force to write manually the serial and improve the performance, this function is more easy but implies lot of memory copy via SIMD
    //send message without reply
    bool packOutcommingData(const uint8_t &packetCode,const char * const data,const unsigned int &size);
    //send query with reply
    bool packOutcommingQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size);
    //send reply
    bool postReplyData(const uint8_t &queryNumber, const char * const data,const int &size);
    //the internal serialiser
    void send_player_move_internal(const uint8_t &moved_unit,const CatchChallenger::Direction &direction) override;
    void Qtlogged(const std::vector<std::vector<CharacterEntry> > &characterEntryList);
protected:
    //have message without reply
    virtual bool parseMessage(const uint8_t &packetCode,const char * const data,const unsigned int &size) override;
    //have query with reply
    virtual bool parseQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size) override;
    //send reply
    virtual bool parseReplyData(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size) override;

    //servers list
    LogicialGroup * addLogicalGroup(const std::string &path, const std::string &xml, const std::string &language);
    ServerFromPoolForDisplay *addLogicalServer(const ServerFromPoolForDisplayTemp &server, const std::string &language);

    //error
    void parseError(const std::string &userMessage, const std::string &errorString);

    //general data
    virtual void defineMaxPlayers(const uint16_t &maxPlayers) = 0;

    //stored local player info
    uint16_t max_players;
    uint16_t max_players_real;
    uint32_t number_of_map;

    //to send trame
    uint8_t queryNumber();
    static std::unordered_set<std::string> extensionAllowed;
    std::map<uint8_t,uint64_t> querySendTime;

    //inventory
    std::vector<uint16_t> lastObjectUsed;

    //datapack
    std::string mDatapackBase;
    std::string mDatapackMain;
    std::string mDatapackSub;

    //teleport list query id
    struct TeleportQueryInformation
    {
        uint8_t queryId;
        Direction direction;//for the internal set of last_direction
    };

    std::vector<TeleportQueryInformation> teleportList;

    //trade
    std::vector<uint8_t> tradeRequestId;
    bool isInTrade;
    //battle
    std::vector<uint8_t> battleRequestId;
    bool isInBattle;
    std::string token;
    std::string passHash;
    std::string loginHash;

    //delayed call info
    std::string login;
    std::string pass;

    //server list
    int32_t selectedServerIndex;
    std::vector<ServerFromPoolForDisplay> serverOrdenedList;
    std::vector<LogicialGroup *> logicialGroupIndexList;
    std::vector<std::vector<CharacterEntry> > characterListForSelection;
    std::string tokenForGameServer;
public:
    virtual void newError(const std::string &error,const std::string &detailedError) = 0;
    virtual void message(const std::string &message) = 0;
    virtual void lastReplyTime(const uint32_t &time) = 0;

    //protocol/connection info
    virtual void disconnected(const std::string &reason) = 0;
    virtual void notLogged(const std::string &reason) = 0;
    virtual void logged(const std::vector<std::vector<CharacterEntry> > &characterEntryList) = 0;//const std::vector<ServerFromPoolForDisplay> &serverOrdenedList,
    virtual void protocol_is_good() = 0;
    virtual void connectedOnLoginServer() = 0;
    virtual void connectingOnGameServer() = 0;
    virtual void connectedOnGameServer() = 0;
    virtual void haveDatapackMainSubCode() = 0;
    virtual void gatewayCacheUpdate(const uint8_t gateway,const uint8_t progression) = 0;

    //general info
    virtual void number_of_player(const uint16_t &number,const uint16_t &max_players) = 0;
    virtual void random_seeds(const std::string &data) = 0;

    //character
    virtual void newCharacterId(const uint8_t &returnCode,const uint32_t &characterId) = 0;
    virtual void haveCharacter() = 0;
    //events
    virtual void setEvents(const std::vector<std::pair<uint8_t,uint8_t> > &events) = 0;
    virtual void newEvent(const uint8_t &event,const uint8_t &event_value) = 0;

    //map move
    virtual void insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction) = 0;
    virtual void move_player(const uint16_t &id,const std::vector<std::pair<uint8_t,CatchChallenger::Direction> > &movement) = 0;
    virtual void remove_player(const uint16_t &id) = 0;
    virtual void reinsert_player(const uint16_t &id,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction) = 0;
    virtual void full_reinsert_player(const uint16_t &id,const uint32_t &mapId,const uint8_t &x,const uint8_t y,const CatchChallenger::Direction &direction) = 0;
    virtual void dropAllPlayerOnTheMap() = 0;
    virtual void teleportTo(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction) = 0;

    //plant
    virtual void insert_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const uint8_t &plant_id,const uint16_t &seconds_to_mature) = 0;
    virtual void remove_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y) = 0;
    virtual void seed_planted(const bool &ok) = 0;
    virtual void plant_collected(const CatchChallenger::Plant_collect &stat) = 0;
    //crafting
    virtual void recipeUsed(const RecipeUsage &recipeUsage) = 0;
    //inventory
    virtual void objectUsed(const ObjectUsage &objectUsage) = 0;
    virtual void monsterCatch(const bool &success) = 0;

    //chat
    virtual void new_chat_text(const CatchChallenger::Chat_type &chat_type,const std::string &text,const std::string &pseudo,const CatchChallenger::Player_type &player_type) = 0;
    virtual void new_system_text(const CatchChallenger::Chat_type &chat_type,const std::string &text) = 0;

    //player info
    virtual void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations) = 0;
    virtual void have_inventory(const std::unordered_map<uint16_t,uint32_t> &items,const std::unordered_map<uint16_t,uint32_t> &warehouse_items) = 0;
    virtual void add_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items) = 0;
    virtual void remove_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items) = 0;

    //datapack
    virtual void haveTheDatapack() = 0;
    virtual void haveTheDatapackMainSub() = 0;
    //base
    virtual void newFileBase(const std::string &fileName,const std::string &data) = 0;
    virtual void newHttpFileBase(const std::string &url,const std::string &fileName) = 0;
    virtual void removeFileBase(const std::string &fileName) = 0;
    virtual void datapackSizeBase(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize) = 0;
    //main
    virtual void newFileMain(const std::string &fileName,const std::string &data) = 0;
    virtual void newHttpFileMain(const std::string &url,const std::string &fileName) = 0;
    virtual void removeFileMain(const std::string &fileName) = 0;
    virtual void datapackSizeMain(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize) = 0;
    //sub
    virtual void newFileSub(const std::string &fileName,const std::string &data) = 0;
    virtual void newHttpFileSub(const std::string &url,const std::string &fileName) = 0;
    virtual void removeFileSub(const std::string &fileName) = 0;
    virtual void datapackSizeSub(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize) = 0;

    //shop
    virtual void haveShopList(const std::vector<ItemToSellOrBuy> &items) = 0;
    virtual void haveBuyObject(const BuyStat &stat,const uint32_t &newPrice) = 0;
    virtual void haveSellObject(const SoldStat &stat,const uint32_t &newPrice) = 0;

    //factory
    virtual void haveFactoryList(const uint32_t &remainingProductionTime,const std::vector<ItemToSellOrBuy> &resources,const std::vector<ItemToSellOrBuy> &products) = 0;
    virtual void haveBuyFactoryObject(const BuyStat &stat,const uint32_t &newPrice) = 0;
    virtual void haveSellFactoryObject(const SoldStat &stat,const uint32_t &newPrice) = 0;

    //trade
    virtual void tradeRequested(const std::string &pseudo,const uint8_t &skinInt) = 0;
    virtual void tradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt) = 0;
    virtual void tradeCanceledByOther() = 0;
    virtual void tradeFinishedByOther() = 0;
    virtual void tradeValidatedByTheServer() = 0;
    virtual void tradeAddTradeCash(const uint64_t &cash) = 0;
    virtual void tradeAddTradeObject(const uint32_t &item,const uint32_t &quantity) = 0;
    virtual void tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster) = 0;

    //battle
    virtual void battleRequested(const std::string &pseudo,const uint8_t &skinInt) = 0;
    virtual void battleAcceptedByOther(const std::string &pseudo,const uint8_t &skinId,const std::vector<uint8_t> &stat,const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster) = 0;
    virtual void battleCanceledByOther() = 0;
    virtual void sendBattleReturn(const std::vector<Skill::AttackReturn> &attackReturn) = 0;

    //clan
    virtual void clanActionSuccess(const uint32_t &clanId) = 0;
    virtual void clanActionFailed() = 0;
    virtual void clanDissolved() = 0;
    virtual void clanInformations(const std::string &name) = 0;
    virtual void clanInvite(const uint32_t &clanId,const std::string &name) = 0;
    virtual void cityCapture(const uint32_t &remainingTime,const uint8_t &type) = 0;

    //city
    virtual void captureCityYourAreNotLeader() = 0;
    virtual void captureCityYourLeaderHaveStartInOtherCity(const std::string &zone) = 0;
    virtual void captureCityPreviousNotFinished() = 0;
    virtual void captureCityStartBattle(const uint16_t &player_count,const uint16_t &clan_count) = 0;
    virtual void captureCityStartBotFight(const uint16_t &player_count,const uint16_t &clan_count,const uint32_t &fightId) = 0;
    virtual void captureCityDelayedStart(const uint16_t &player_count,const uint16_t &clan_count) = 0;
    virtual void captureCityWin() = 0;
public:
    virtual std::string getLanguage() const = 0;
public:
    virtual void send_player_direction(const CatchChallenger::Direction &the_direction);
    void send_player_move(const uint8_t &moved_unit,const CatchChallenger::Direction &direction);
    //void send_move_unit(const CatchChallenger::Direction &direction);->do by MoveOnTheMap::newDirection()
    void sendChatText(const CatchChallenger::Chat_type &chatType,const std::string &text);
    void sendPM(const std::string &text,const std::string &pseudo);
    bool teleportDone();

    //character
    bool addCharacter(const uint8_t &charactersGroupIndex,const uint8_t &profileIndex, const std::string &pseudo, const uint8_t &monsterGroupId, const uint8_t &skinId);
    bool removeCharacter(const uint8_t &charactersGroupIndex,const uint32_t &characterId);
    bool selectCharacter(const uint8_t &charactersGroupIndex, const uint32_t &serverUniqueKey, const uint32_t &characterId);
    bool selectCharacter(const uint8_t &charactersGroupIndex, const uint32_t &serverUniqueKey, const uint32_t &characterId,const uint32_t &serverIndex);
    bool selectCharacter(const uint32_t &serverIndex, const uint32_t &characterId);
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
    void addTradeCash(const uint64_t &cash);
    void addObject(const uint16_t &item,const uint32_t &quantity);
    void addMonsterByPosition(const uint8_t &monsterPosition);

    //battle
    void battleRefused();
    void battleAccepted();

    //inventory
    void destroyObject(const uint16_t &object,const uint32_t &quantity=1);
    bool useObject(const uint16_t &object);
    bool useObjectOnMonsterByPosition(const uint16_t &object, const uint8_t &monsterPosition);
    bool wareHouseStore(const uint64_t &withdrawCash, const uint64_t &depositeCash,
                        const std::vector<std::pair<uint16_t,uint32_t> > &withdrawItems, const std::vector<std::pair<uint16_t,uint32_t> > &depositeItems,
                        const std::vector<uint8_t> &withdrawMonsters, const std::vector<uint8_t> &depositeMonsters);
    void takeAnObjectOnMap();

    //shop
    void getShopList(const uint16_t &shopId);/// \see CommonMap, std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>, pairhash> shops;
    void buyObject(const uint16_t &shopId,const uint16_t &objectId,const uint32_t &quantity,const uint32_t &price);/// \see CommonMap, std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>, pairhash> shops;
    void sellObject(const uint16_t &shopId, const uint16_t &objectId, const uint32_t &quantity, const uint32_t &price);/// \see CommonMap, std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>, pairhash> shops;

    //factory
    void getFactoryList(const uint16_t &factoryId);
    void buyFactoryProduct(const uint16_t &factoryId,const uint16_t &objectId,const uint32_t &quantity,const uint32_t &price);
    void sellFactoryResource(const uint16_t &factoryId, const uint16_t &objectId, const uint32_t &quantity, const uint32_t &price);

    //fight
    void sendTryEscape();
    void useSkill(const uint16_t &skill);
    void heal();
    void requestFight(const uint16_t &fightId);
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
    void createClan(const std::string &name);
    void leaveClan();
    void dissolveClan();
    void inviteClan(const std::string &pseudo);
    void ejectClan(const std::string &pseudo);
    void inviteAccept(const bool &accept);
    void waitingForCityCapture(const bool &cancel);

    //warehouse
    void addPlayerMonsterWarehouse(const PlayerMonster &playerMonster);
    bool removeMonsterWarehouseByPosition(const uint8_t &monsterPosition);
};
}

#endif
#endif // CATCHCHALLENGER_PROTOCOL_H
