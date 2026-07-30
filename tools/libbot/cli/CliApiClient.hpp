#ifndef CATCHCHALLENGER_CLI_CLIAPICLIENT_H
#define CATCHCHALLENGER_CLI_CLIAPICLIENT_H

//ClientStructures.hpp (pulled by Api_protocol.hpp) names tinyxml2::XMLElement
//in a member type; forward-declare it so the pointer compiles even in a
//CATCHCHALLENGER_NOXML build.
namespace tinyxml2 { class XMLElement; }

#include "../../../client/libcatchchallenger/Api_protocol.hpp"
#include "CliSocket.hpp"

#include <stdint.h>
#include <string>
#include <vector>

namespace CatchChallenger {

/** \brief Qt-free concrete Api_protocol: one bot, one TCP socket.
 *
 * Everything the GUI clients do with Qt signals happens here as a plain
 * state variable, so a CLI event loop can drive N of these with select().
 *
 * The login is TWO-STAGE and this class walks it end to end:
 *   Stage1  connect to the LOGIN server, readForFirstHeader(), sendProtocol()
 *           -> 0xA0 reply -> Api_protocol auto-calls tryLogin() (0xA8)
 *   0xA8 reply -> logged() -> selectCharacter() (0xAC), or addCharacter()
 *           (0xAA) then selectCharacter() when newCharacterId() lands
 *   0xAC reply carries the GAME server host+token -> Stage2: Api_protocol
 *           closes the socket and calls connectingOnGameServer()
 *   Stage3  socketDisconnectedForReconnect() gives "host:port", connect a NEW
 *           fd there, readForFirstHeader() again, sendProtocol() flips to
 *           Stage4 and its reply auto-sends 0x93 with the token
 *   the 0x93 reply is the character block: it needs the datapack parsed, so
 *           haveDatapackMainSubCode() only RECORDS the codes and the event
 *           loop calls loadDatapackAndReplay() after parsing returns
 *   haveCharacter() = the bot is on the map.
 *
 * Two things are deliberately NOT done inside a protocol callback, because
 * they would re-enter the parser: the socket reconnect and the datapack load.
 * Both are flagged and executed by the event loop through pendingWork().
 */
class CliApiClient : public Api_protocol
{
public:
    enum State : uint8_t
    {
        State_Idle=0,
        State_ConnectingLogin=1,
        State_LoginProtocol=2,
        State_LoggingIn=3,
        State_CreatingCharacter=4,
        State_SelectingCharacter=5,
        State_NeedReconnect=6,
        State_ConnectingGame=7,
        State_GameProtocol=8,
        State_NeedDatapack=9,
        State_OnMap=10,
        State_Failed=11,
        /// \brief terminal state of the --list-only mode: the login reply was
        /// received and its character list counted, no character selected.
        State_Listed=12
    };
    /// \brief work the event loop must run OUTSIDE the parser
    enum PendingWork : uint8_t
    {
        PendingWork_None=0,
        PendingWork_Reconnect=1,
        PendingWork_LoadDatapack=2
    };

    CliApiClient();
    virtual ~CliApiClient();

    /// \brief login/pass/datapack path; call before connectToLoginServer()
    void setIdentity(const std::string &login,const std::string &pass,
                     const std::string &datapackPath);
    /// \brief which character of the account this bot takes.
    /// N bots share ONE account, so bot i claims the i-th character of the
    /// login reply and creates one when the account has fewer than i+1.
    /// Two bots on the same character would get "Already logged".
    void setCharacterSlot(const uint32_t &slot);
    /// \brief stop right after the login reply instead of taking a character.
    /// Only the size of the returned character list is kept
    /// (getCharacterCount()); nothing is selected nor created, so a run in
    /// this mode cannot collide with another client on a character.
    void setListOnly(const bool &listOnly);
    /// \brief characters the login reply listed, summed over every characters
    /// group. Meaningful once getState()==State_Listed.
    uint32_t getCharacterCount() const;
    /// \brief start the non-blocking connect to the LOGIN server
    bool connectToLoginServer(const std::string &host,const uint16_t &port);

    //---- event-loop interface -------------------------------------------
    CliSocket &getSocket();
    State getState() const;
    static const char *stateToString(const State &state);
    /// \brief the connect finished (fd writable): check it and send 0xA0
    void socketWritable();
    /// \brief bytes are pending: run the production protocol parser
    void socketReadyRead();
    PendingWork getPendingWork() const;
    /// \brief run the flagged deferred work; false when it failed
    bool runPendingWork();
    /// \brief true once the bot is on the map or permanently failed
    bool isFinished() const;
    const std::string &getFailReason() const;
    /// \brief set when getState()==State_OnMap
    CATCHCHALLENGER_TYPE_MAPID getMapIndex() const;
    COORD_TYPE getX() const;
    COORD_TYPE getY() const;
    /// \brief printed by the caller on every state change, then cleared
    bool takeStateChanged();
    /// \brief bot label used in logs and as the created character pseudo
    const std::string &getLabel() const;
    void setLabel(const std::string &label);
    /// \brief print every state transition and protocol message when true
    void setVerbose(const bool &verbose);

    //---- transport seam (ProtocolParsing) ------------------------------
    ssize_t readFromSocket(char *data,const size_t &size) override;
    ssize_t writeToSocket(const char * const data,const size_t &size) override;
    void closeSocket() override;

    //---- Api_protocol hooks that carry the state machine ---------------
    void readForFirstHeader() override;
    void tryDisconnect() override;
    void defineMaxPlayers(const uint16_t &maxPlayers) override;
    void newError(const std::string &error,const std::string &detailedError) override;
    void message(const std::string &message) override;
    void disconnected(const std::string &reason) override;
    void notLogged(const std::string &reason) override;
    void logged(const std::vector<std::vector<CharacterEntry> > &characterEntryList) override;
    void protocol_is_good() override;
    void connectedOnLoginServer() override;
    void connectingOnGameServer() override;
    void connectedOnGameServer() override;
    void haveDatapackMainSubCode() override;
    void newCharacterId(const uint8_t &returnCode,const uint32_t &characterId) override;
    void haveCharacter(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,
                       const COORD_TYPE &y,const Direction &last_direction) override;
    std::string getLanguage() const override;
    bool haveBeatBot(const CATCHCHALLENGER_TYPE_MAPID &mapId,
                     const CATCHCHALLENGER_TYPE_BOTID &botFightId) const override;

    //---- Api_protocol notifications a benchmark bot ignores ------------
    void lastReplyTime(const uint32_t &time) override;
    void gatewayCacheUpdate(const uint8_t gateway,const uint8_t progression) override;
    void number_of_player(const PLAYER_INDEX_FOR_CONNECTED &number,const PLAYER_INDEX_FOR_CONNECTED &max_players) override;
    void random_seeds(const std::string &data) override;
    void setEvents(const std::vector<std::pair<uint8_t,uint8_t> > &events) override;
    void newEvent(const uint8_t &event,const uint8_t &event_value) override;
    void insert_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex,const Player_public_informations &player,const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const Direction &direction) override;
    void move_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex,const std::vector<std::pair<uint8_t,Direction> > &movement) override;
    void remove_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex) override;
    void reinsert_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex,const uint8_t &x,const uint8_t &y,const Direction &direction) override;
    void full_reinsert_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex,const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE y,const Direction &direction) override;
    void dropAllPlayerOnTheMap() override;
    void teleportTo(const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const Direction &direction) override;
    void recipeUsed(const RecipeUsage &recipeUsage) override;
    void objectUsed(const ObjectUsage &objectUsage) override;
    void monsterCatch(const bool &success) override;
    void new_chat_text(const Chat_type &chat_type,const std::string &text,const std::string &pseudo,const Player_type &player_type) override;
    void new_system_text(const Chat_type &chat_type,const std::string &text) override;
    void have_current_player_info(const Player_private_and_public_informations &informations) override;
    void have_inventory(const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t> &items) override;
    void add_to_inventory(const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t> &items) override;
    void remove_to_inventory(const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t> &items) override;
    void haveTheDatapack() override;
    void haveTheDatapackMainSub() override;
    void newFileBase(const std::string &fileName,const std::string &data) override;
    void newHttpFileBase(const std::string &url,const std::string &fileName) override;
    void removeFileBase(const std::string &fileName) override;
    void datapackSizeBase(const DATAPACK_FILE_NUMBER &datapckFileNumber,const uint32_t &datapckFileSize) override;
    void newFileMain(const std::string &fileName,const std::string &data) override;
    void newHttpFileMain(const std::string &url,const std::string &fileName) override;
    void removeFileMain(const std::string &fileName) override;
    void datapackSizeMain(const DATAPACK_FILE_NUMBER &datapckFileNumber,const uint32_t &datapckFileSize) override;
    void newFileSub(const std::string &fileName,const std::string &data) override;
    void newHttpFileSub(const std::string &url,const std::string &fileName) override;
    void removeFileSub(const std::string &fileName) override;
    void datapackSizeSub(const DATAPACK_FILE_NUMBER &datapckFileNumber,const uint32_t &datapckFileSize) override;
    void haveShopList(const std::vector<ItemToSellOrBuy> &items) override;
    void haveBuyObject(const BuyStat &stat,const uint32_t &newPrice) override;
    void haveSellObject(const SoldStat &stat,const uint32_t &newPrice) override;
    void haveFactoryList(const uint32_t &remainingProductionTime,const std::vector<ItemToSellOrBuy> &resources,const std::vector<ItemToSellOrBuy> &products) override;
    void haveBuyFactoryObject(const BuyStat &stat,const uint32_t &newPrice) override;
    void haveSellFactoryObject(const SoldStat &stat,const uint32_t &newPrice) override;
    void tradeRequested(const std::string &pseudo,const uint8_t &skinInt) override;
    void tradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt) override;
    void tradeCanceledByOther() override;
    void tradeFinishedByOther() override;
    void tradeValidatedByTheServer() override;
    void tradeAddTradeCash(const uint64_t &cash) override;
    void tradeAddTradeObject(const CATCHCHALLENGER_TYPE_ITEM &item,const uint32_t &quantity) override;
    void tradeAddTradeMonster(const PlayerMonster &monster) override;
    void battleRequested(const std::string &pseudo,const uint8_t &skinInt) override;
    void battleAcceptedByOther(const std::string &pseudo,const uint8_t &skinId,const std::vector<uint8_t> &stat,const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster) override;
    void battleCanceledByOther() override;
    void sendBattleReturn(const std::vector<Skill::AttackReturn> &attackReturn) override;
    void clanActionSuccess(const uint32_t &clanId) override;
    void clanActionFailed() override;
    void clanDissolved() override;
    void clanInformations(const std::string &name) override;
    void clanInvite(const uint32_t &clanId,const std::string &name) override;
    void cityCapture(const uint32_t &remainingTime,const uint8_t &type) override;
    void captureCityYourAreNotLeader() override;
    void captureCityYourLeaderHaveStartInOtherCity(const std::string &zone) override;
    void captureCityPreviousNotFinished() override;
    void captureCityStartBattle(const PLAYER_INDEX_FOR_CONNECTED &player_count,const uint16_t &clan_count) override;
    void captureCityStartBotFight(const PLAYER_INDEX_FOR_CONNECTED &player_count,const uint16_t &clan_count,const uint32_t &fightId) override;
    void captureCityDelayedStart(const PLAYER_INDEX_FOR_CONNECTED &player_count,const uint16_t &clan_count) override;
    void captureCityWin() override;
private:
    void setState(const State &state);
    void fail(const std::string &reason);
    /// \brief pick a character from the login reply, or ask to create one
    void selectOrCreateCharacter(const std::vector<std::vector<CharacterEntry> > &characterEntryList);
    bool createCharacter();
    /// \brief close the login socket and connect to the game server
    bool doReconnect();
    /// \brief parse the datapack then release the queued character block
    bool doLoadDatapack();

    CliSocket socket;
    State state;
    PendingWork pendingWork;
    std::string identityLogin;
    std::string identityPass;
    std::string datapackPath;
    std::string failReason;
    std::string label;
    bool verbose;
    bool stateChanged;
    bool listOnly;
    /// \brief charactersGroupIndex the selected/created character belongs to
    uint8_t charactersGroupIndex;
    uint32_t characterSlot;
    uint32_t characterCount;
    CATCHCHALLENGER_TYPE_MAPID mapIndex;
    COORD_TYPE x;
    COORD_TYPE y;
};

}

#endif // CATCHCHALLENGER_CLI_CLIAPICLIENT_H
