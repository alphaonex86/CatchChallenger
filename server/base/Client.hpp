#ifndef CATCHCHALLENGER_CLIENT_H
#define CATCHCHALLENGER_CLIENT_H

#include <vector>

#include "ServerStructures.hpp"
#include "ClientMapManagement/ClientMapManagement.hpp"
#include "BroadCastWithoutSender.hpp"
#include "BaseServerMasterSendDatapack.hpp"
#include "../../general/base/GeneralStructures.hpp"
#include "../../general/fight/CommonFightEngine.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/ProtocolParsing.hpp"
#include "DdosBuffer.hpp"
#include <unordered_map>
#include <vector>
#include <queue>
#include <iostream>

#ifdef EPOLLCATCHCHALLENGERSERVER
    #ifdef CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB
        #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB*1024
        #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB
        #endif
        #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB*1024
        #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB
        #endif
        #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB*1024
        #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB
        #endif
        #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_XZ_COMPRESSEDFILEPURGE_KB*1024
        #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_XZ_COMPRESSEDFILEPURGE_KB
        #endif
        #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_DONT_COMPRESS_GREATER_THAN_KB*1024
        #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_DONT_COMPRESS_GREATER_THAN_KB
        #endif
    #endif
#endif

namespace CatchChallenger {

#ifdef EPOLLCATCHCHALLENGERSERVER
void recordDisconnectByServer(void * client);
#endif

class Client : public ProtocolParsingInputOutput, public CommonFightEngine, public ClientMapManagement, public ClientBase
{
public:
    friend class BaseServer;
    friend class PlayerUpdaterBase;
    explicit Client();
    virtual ~Client();
    //to get some info
    virtual bool isValid() = 0;
    std::string getPseudo() const;
    void savePosition();
    static bool characterConnected(const uint32_t &characterId);
    bool disconnectClient() override;
    static void disconnectClientById(const uint32_t &characterId);
    Client *getClientFight() const;
    bool triggerDaillyGift(const uint64_t &timeRangeEventTimestamps);//return true if validated and gift sended
    #ifdef CATCHCHALLENGER_DDOS_FILTER
    void doDDOSCompute();
    #endif
    void breakNeedMoreData() override;
    void receive_instant_player_number(const uint16_t &connected_players, const char * const data, const uint8_t &size);
    virtual void parseIncommingData() override;
    static void startTheCityCapture();
    static void setEvent(const uint8_t &event, const uint8_t &new_value);
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    static char * addAuthGetToken(const uint32_t &characterId,const uint32_t &accountIdRequester);
    static void purgeTokenAuthList();
    #endif
    static void timeRangeEvent(const uint64_t &timestamps);
    uint32_t getPlayerId() const;
    uint32_t getClanId() const;
    bool haveAClan() const;

    bool sendRawBlock(const char * const data,const unsigned int &size);

    static DdosBuffer<uint16_t,3> generalChatDrop;
    static DdosBuffer<uint16_t,3> clanChatDrop;
    static DdosBuffer<uint16_t,3> privateChatDrop;
    static std::vector<uint32_t> marketObjectUniqueIdList;//this is not the item id
    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    static uint64_t datapack_list_cache_timestamp_base,datapack_list_cache_timestamp_main,datapack_list_cache_timestamp_sub;
    #endif
    static std::vector<uint16_t> simplifiedIdList;
    static std::unordered_map<std::string,Client *> playerByPseudo;
    static std::unordered_map<uint32_t,Clan *> clanList;
    static std::vector<Client *> clientBroadCastList;
    static bool timeRangeEventNew;
    static uint64_t timeRangeEventTimestamps;

    static unsigned char protocolReplyProtocolNotSupported[7];
    static unsigned char protocolReplyServerFull[7];
    static unsigned char *characterIsRightFinalStepHeader;
    static uint32_t characterIsRightFinalStepHeaderSize;
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    /// \todo group all reply in one
    static unsigned char protocolReplyCompressionNone[7+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    static unsigned char protocolReplyCompresssionZstandard[7+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    #endif
    static std::vector<Client *> stat_client_list;
    static unsigned char private_token_statclient[TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];

    static unsigned char *protocolReplyCharacterList;
    static uint16_t protocolReplyCharacterListSize;
private:
    static unsigned char *protocolMessageLogicalGroupAndServerList;
    static uint16_t protocolMessageLogicalGroupAndServerListSize;
    static uint16_t protocolMessageLogicalGroupAndServerListPosPlayerNumber;
public:
    #else
    static unsigned char protocolReplyCompressionNone[7];
    static unsigned char protocolReplyCompresssionZstandard[7];
    #endif

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    static unsigned char loginIsWrongBuffer[7];
    #endif

    static const unsigned char protocolHeaderToMatch[5];

    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    struct TokenAuth
    {
        char *token;
        uint32_t characterId;
        uint32_t accountIdRequester;
        uint64_t createTime;
    };
    static std::vector<TokenAuth> tokenAuthList;
    #endif
protected:
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    bool stat_client;
    #endif
    enum ClientStat : uint8_t
    {
        None=0x00,
        ProtocolGood=0x01,
        LoginInProgress=0x02,
        Logged=0x03,
        LoggedStatClient=0x04,
        CharacterSelecting=0x05,
        CharacterSelected=0x06,
    };
    ClientStat stat;
    uint64_t lastdaillygift;//datalocallity with ClientStat stat

    std::queue<CatchChallenger::DatabaseBaseCallBack *> callbackRegistred;

    struct ClanActionParam
    {
        uint8_t query_id;
        uint8_t action;
        std::string text;
    };
    struct AddCharacterParam
    {
        uint8_t query_id;
        uint8_t profileIndex;
        std::string pseudo;
        uint8_t monsterGroupId;
        uint8_t skinId;
    };
    struct RemoveCharacterParam
    {
        uint8_t query_id;
        uint32_t characterId;
    };
    struct DeleteCharacterNow
    {
        uint32_t characterId;
    };
    struct AskLoginParam
    {
        uint8_t query_id;
        char login[CATCHCHALLENGER_SHA224HASH_SIZE];
        char pass[CATCHCHALLENGER_SHA224HASH_SIZE];
        //to store the reply to the char, and do another query
        char * characterOutputData;
        uint32_t characterOutputDataSize;
    };
    struct SelectCharacterParam
    {
        uint8_t query_id;
        uint32_t characterId;
    };
    struct SelectIndexParam
    {
        uint32_t index;
    };
private:
    //-------------------
    uint32_t account_id;//0 if not logged
    uint32_t character_id;
    uint64_t market_cash;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    bool isConnected;
    #endif
    uint16_t randomIndex,randomSize;
    uint8_t number_of_character;

    PlayerOnMap map_entry;
    PlayerOnMap rescue;
    PlayerOnMap unvalidated_rescue;
    std::unordered_map<uint32_t,std::vector<MonsterDrops> > questsDrop;
    uint64_t connectedSince;
    struct OldEvents
    {
        struct OldEventEntry
        {
            uint8_t event;
            uint8_t eventValue;
        };
        uint64_t time;
        std::vector<OldEventEntry> oldEventList;
    };
    OldEvents oldEvents;

    enum DatapackStatus : uint8_t
    {
        Base=0x01,
        Main=0x02,
        Sub=0x03,
        Finished=0x04
    };
    DatapackStatus datapackStatus;

    int32_t connected_players;//it's th last number of connected player send
    std::queue<void *> paramToPassToCallBack;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    std::queue<std::string > paramToPassToCallBackType;
    #endif
    static std::vector<uint8_t> selectCharacterQueryId;

    // for status
    bool stopIt;
    #ifdef CATCHCHALLENGER_DDOS_FILTER
    DdosBuffer<uint8_t,3> movePacketKick;
    DdosBuffer<uint8_t,3> chatPacketKick;
    DdosBuffer<uint8_t,3> otherPacketKick;
    #endif
    uint8_t profileIndex;
    std::queue<PlayerOnMap> lastTeleportation;
    std::vector<uint8_t> queryNumberList;

    Client *otherPlayerBattle;
    bool battleIsValidated;
    uint16_t mCurrentSkillId;
    bool mHaveCurrentSkill,mMonsterChange;
    uint64_t botFightCash;
    uint16_t botFightId;
    bool isInCityCapture;
    std::vector<Skill::AttackReturn> attackReturn;
    //std::unordered_map<uint32_t/*currentMonster->id*/, std::unordered_map<uint32_t/*skill*/,uint32_t> > deferedEnduranceSync;
    std::unordered_set<PlayerMonster *> deferedEnduranceSync;

    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    static uint8_t tempDatapackListReplySize;
    static std::vector<char> tempDatapackListReplyArray;
    static uint8_t tempDatapackListReply;
    static unsigned int tempDatapackListReplyTestCount;
    //static std::unordered_map<std::string,uint32_t> datapack_file_list_cache_base,datapack_file_list_cache_main,datapack_file_list_cache_sub;//same than above
    static std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> /*do into BaseServerMasterSendDatapack::datapack_file_hash_cache_base,*/datapack_file_hash_cache_main,datapack_file_hash_cache_sub;
    #endif
    static std::regex fileNameStartStringRegex;

    //info linked
    static Direction	temp_direction;
    static std::unordered_map<uint32_t,Client *> playerById;
    static std::unordered_map<uint16_t,std::vector<Client *> > captureCity;
    static std::unordered_map<uint16_t,CaptureCityValidated> captureCityValidatedList;
    static std::unordered_map<uint32_t,uint64_t> characterCreationDateList;

    //socket related
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //void connectionError(QAbstractSocket::SocketError);
    #endif

    #ifndef EPOLLCATCHCHALLENGERSERVER
    /// \warning it need be complete protocol trame
    void fake_receive_data(std::vector<char> data);
    #endif
    //global slot
    bool sendPM(const std::string &text,const std::string &pseudo);
    bool receiveChatText(const Chat_type &chatType, const std::string &text, const Client *sender_informations);
    bool receiveSystemText(const std::string &text,const bool &important=false);
    bool sendChatText(const Chat_type &chatType,const std::string &text);
    void sendBroadCastCommand(const std::string &command,const std::string &extraText);
    void setRights(const Player_type& type);
    //normal slots
    void sendSystemMessage(const std::string &text,const bool &important=false,const bool &playerInclude=false);
    //clan
    void clanChangeWithoutDb(const uint32_t &clanId);

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    bool askLogin(const uint8_t &query_id, const char *rawdata);
    bool createAccount(const uint8_t &query_id, const char *rawdata);
    static void createAccount_static(void *object);
    void createAccount_object();
    void createAccount_return(AskLoginParam *askLoginParam);
    static void askLogin_static(void *object);
    void askLogin_object();
    void askLogin_return(AskLoginParam *askLoginParam);
    static void character_list_static(void *object);
    void character_list_object();
    uint32_t character_list_return(char *data, const uint8_t &query_id);
    bool server_list();
    static void server_list_static(void *object);
    void server_list_object();
    void server_list_return(const uint8_t &query_id, const char * const characterOutputData, const uint32_t &characterOutputDataSize);
    void deleteCharacterNow(const uint32_t &characterId);
    static void deleteCharacterNow_static(void *object);
    void deleteCharacterNow_object();
    void deleteCharacterNow_return(const uint32_t &characterId);
    #endif
    static std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> datapack_file_list(const std::string &path,const std::string &exclude,const bool withHash=true);//used into BaseServer to do the hash
    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    //check each element of the datapack, determine if need be removed, updated, add as new file all the missing file
    void datapackList(const uint8_t &query_id, const std::vector<std::string > &files, const std::vector<uint32_t> &partialHashList);
    std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> datapack_file_list_cached_base();
    std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> datapack_file_list_cached_main();
    std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> datapack_file_list_cached_sub();
    void addDatapackListReply(const bool &fileRemove);
    void purgeDatapackListReply(const uint8_t &query_id);
    void sendFileContent();
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    void sendCompressedFileContent();
    #endif
    #endif
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    void dbQueryWriteLogin(const std::string &queryText);
    #endif
    void dbQueryWriteCommon(const std::string &queryText);
    void dbQueryWriteServer(const std::string &queryText);
    //character
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    void addCharacter(const uint8_t &query_id, const uint8_t &profileIndex, const std::string &pseudo, const uint8_t &monsterGroupId, const uint8_t &skinId);
    static void addCharacter_static(void *object);
    void addCharacter_object();
    void addCharacter_return(const uint8_t &query_id, const uint8_t &profileIndex, const std::string &pseudo, const uint8_t &monsterGroupId, const uint8_t &skinId);
    void removeCharacterLater(const uint8_t &query_id, const uint32_t &characterId);
    static void removeCharacterLater_static(void *object);
    void removeCharacterLater_object();
    void removeCharacterLater_return(const uint8_t &query_id, const uint32_t &characterId);
    #endif
    void selectCharacter(const uint8_t &query_id, const uint32_t &characterId);
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    void selectCharacter(const uint8_t &query_id, const char * const token);
    #endif
    static void selectCharacter_static(void *object);
    void selectCharacter_object();
    void selectCharacter_return(const uint8_t &query_id, const uint32_t &characterId);
    void selectCharacterServer(const uint8_t &query_id, const uint32_t &characterId, const uint64_t &characterCreationDate);
    static void selectCharacterServer_static(void *object);
    void selectCharacterServer_object();
    void selectCharacterServer_return(const uint8_t &query_id, const uint32_t &characterId);

    static void selectClan_static(void *object);
    void selectClan_object();
    void selectClan_return();

    void fake_receive_data(const std::vector<char> &);
    void purgeReadBuffer();

    void sendNewEvent(char * const data, const uint32_t &size);
    void teleportTo(CommonMap *map,const /*COORD_TYPE*/uint8_t &x,const /*COORD_TYPE*/uint8_t &y,const Orientation &orientation);
    void sendTradeRequest(char * const data,const uint32_t &size);
    void sendBattleRequest(char * const data,const uint32_t &size);

    //chat
    void sendLocalChatText(const std::string &text);
    //seed
    void seedValidated();
    void plantSeed(const uint8_t &plant_id);
    void collectPlant();

    void createMemoryClan();
    Direction lookToMove(const Direction &direction);
    //seed
    void useSeed(const uint8_t &plant_id);
    //crafting
    void useRecipe(const uint8_t &query_id, const uint16_t &recipe_id);
    void takeAnObjectOnMap();
    //inventory
    void addObjectAndSend(const uint16_t &item,const uint32_t &quantity=1);
    void addObject(const uint16_t &item,const uint32_t &quantity=1,const bool databaseSync=true);
    void saveObjectRetention(const uint16_t &item);
    uint32_t removeObject(const uint16_t &item,const uint32_t &quantity=1,const bool databaseSync=true);
    void sendRemoveObject(const uint16_t &item,const uint32_t &quantity=1);
    void updateObjectInDatabase();
    //void updateMonsterInDatabase();
    void updateObjectInWarehouseDatabase();
    //void updateMonsterInWarehouseDatabase();
    void updateObjectInDatabaseAndEncyclopedia();
    void updateMonsterInDatabaseEncyclopedia();
    uint32_t objectQuantity(const uint16_t &item) const;
    bool addMarketCashWithoutSave(const uint64_t &cash);
    void addCash(const uint64_t &cash,const bool &forceSave=false);
    void removeCash(const uint64_t &cash);
    void addWarehouseCash(const uint64_t &cash,const bool &forceSave=false);
    void removeWarehouseCash(const uint64_t &cash);
    bool wareHouseStore(const uint64_t &withdrawCash, const uint64_t &depositeCash, const std::vector<std::pair<uint16_t, uint32_t> > &withdrawItems, const std::vector<std::pair<uint16_t, uint32_t> > &depositeItems, const std::vector<uint8_t> &withdrawMonsters, const std::vector<uint8_t> &depositeMonsters);
    bool wareHouseStoreCheck(const uint64_t &withdrawCash, const uint64_t &depositeCash,
                             const std::vector<std::pair<uint16_t,uint32_t> > &withdrawItems, const std::vector<std::pair<uint16_t,uint32_t> > &depositeItems,
                             const std::vector<uint8_t> &withdrawMonsters, const std::vector<uint8_t> &depositeMonsters);
    void addWarehouseObject(const uint16_t &item,const uint32_t &quantity=1,const bool databaseSync=true);
    uint32_t removeWarehouseObject(const uint16_t &item,const uint32_t &quantity=1,const bool databaseSync=true);

    bool haveReputationRequirements(const std::vector<ReputationRequirements> &reputationList) const;
    void confirmEvolution(const uint8_t &monsterPosition);
    void sendHandlerCommand(const std::string &command,const std::string &extraText);
    void addEventInQueue(const uint8_t &event, const uint8_t &event_value, const uint64_t &currentDateTime);
    void removeFirstEventInQueue();
    //inventory
    void destroyObject(const uint16_t &itemId,const uint32_t &quantity);
    void useObject(const uint8_t &query_id,const uint16_t &itemId);
    bool useObjectOnMonsterByPosition(const uint16_t &object, const uint8_t &monsterPosition) override;
    //teleportation
    void receiveTeleportTo(CommonMap *map,const /*COORD_TYPE*/uint8_t &x,const /*COORD_TYPE*/uint8_t &y,const Orientation &orientation);
    //shop
    void getShopList(const uint8_t &query_id, const SHOP_TYPE &shopId);
    void buyObject(const uint8_t &query_id, const SHOP_TYPE &shopId, const CATCHCHALLENGER_TYPE_ITEM &objectId, const CATCHCHALLENGER_TYPE_ITEM_QUANTITY &quantity, const uint32_t &price);
    void sellObject(const uint8_t &query_id,const SHOP_TYPE &shopId,const CATCHCHALLENGER_TYPE_ITEM &objectId,const CATCHCHALLENGER_TYPE_ITEM_QUANTITY &quantity,const uint32_t &price);
    //factory
    void saveIndustryStatus(const FACTORY_TYPE &factoryId, const IndustryStatus &industryStatus, const Industry &industry);
    void getFactoryList(const uint8_t &query_id, const FACTORY_TYPE &factoryId);
    void buyFactoryProduct(const uint8_t &query_id,const FACTORY_TYPE &factoryId,const CATCHCHALLENGER_TYPE_ITEM &objectId,const CATCHCHALLENGER_TYPE_ITEM_QUANTITY &quantity,const uint32_t &price);
    void sellFactoryResource(const uint8_t &query_id,const FACTORY_TYPE &factoryId,const CATCHCHALLENGER_TYPE_ITEM &objectId,const CATCHCHALLENGER_TYPE_ITEM_QUANTITY &quantity,const uint32_t &price);
    //trade
    void tradeCanceled();
    void tradeAccepted();
    void tradeFinished();
    void tradeAddTradeCash(const uint64_t &cash);
    void tradeAddTradeObject(const uint16_t &item,const uint32_t &quantity);
    void tradeAddTradeMonster(const uint8_t &monsterPosition);
    //quest
    void newQuestAction(const QuestAction &action, const uint16_t &questId);
    void appendAllow(const ActionAllow &allow);
    void removeAllow(const ActionAllow &allow);
    void syncDatabaseAllow();
    //plant
    bool syncDatabasePlant();
    //item on map
    bool syncDatabaseItemOnMap();
    //reputation
    void appendReputationPoint(const uint8_t &reputationId, const int32_t &point);
    void appendReputationRewards(const std::vector<ReputationRewards> &reputationList);
    void syncDatabaseReputation();
    //bot
    void syncBotAlreadyBeaten();
    //battle
    void battleCanceled();
    void battleAccepted();
    virtual bool tryEscape() override;
    void heal();
    void requestFight(const uint16_t &fightId);
    bool learnSkillByMonsterPosition(const uint8_t &monsterPosition,const uint16_t &skill);
    Client * getLocalClientHandlerFight();
    //clan
    void clanAction(const uint8_t &query_id,const uint8_t &action,const std::string &text);
    void addClan_return(const uint8_t &query_id, const uint8_t &action, const std::string &text);
    void addClan_object();
    static void addClan_static(void *object);
    void haveClanInfo(const uint32_t &clanId, const std::string &clanName, const uint64_t &cash);
    void sendClanInfo();
    void clanInvite(const bool &accept);
    uint32_t clanId() const;
    void removeFromClan();
    void waitingForCityCaputre(const bool &cancel);
    void previousCityCaptureNotFinished();
    void leaveTheCityCapture();
    void cityCaptureBattle(const uint16_t &number_of_player,const uint16_t &number_of_clan);
    void cityCaptureBotFight(const uint16_t &number_of_player,const uint16_t &number_of_clan,const uint32_t &fightId);
    void cityCaptureInWait(const uint16_t &number_of_player,const uint16_t &number_of_clan);
    void cityCaptureWin();
    static void cityCaptureSendInWait(const CaptureCityValidated &captureCityValidated,const uint16_t &number_of_player,const uint16_t &number_of_clan);
    static uint16_t cityCapturePlayerCount(const CaptureCityValidated &captureCityValidated);
    static uint16_t cityCaptureClanCount(const CaptureCityValidated &captureCityValidated);
    void setInCityCapture(const bool &isInCityCapture);
    //map move
    bool captureCityInProgress();
    void fightOrBattleFinish(const bool &win, const uint16_t &fightId);//fightId == 0 if is in battle
    void moveMonster(const bool &up,const uint8_t &number);
    //market
    void getMarketList(const uint8_t &query_id);
    void buyMarketObject(const uint8_t &query_id,const uint32_t &marketObjectId,const uint32_t &quantity);
    void buyMarketMonster(const uint8_t &query_id, const uint32_t &marketMonsterUniqueId/*To ident even if the position have changed, imply search at server*/);
    void putMarketObject(const uint8_t &query_id, const uint16_t &objectId, const uint32_t &quantity, const uint64_t &price);
    void putMarketMonster(const uint8_t &query_id, const uint8_t &monsterPosition, const uint64_t &price);
    void withdrawMarketCash(const uint8_t &query_id);
    void withdrawMarketObject(const uint8_t &query_id,const uint16_t &objectId,const uint32_t &quantity);
    void withdrawMarketMonster(const uint8_t &query_id, const uint32_t &marketMonsterUniqueId/*To ident even if the position have changed, imply search at server*/);

    Player_private_and_public_informations &get_public_and_private_informations() override;
    const Player_private_and_public_informations &get_public_and_private_informations_ro() const override;
    static std::string directionToStringToSave(const Direction &direction);
    static std::string orientationToStringToSave(const Orientation &orientation);
    //quest
    bool haveNextStepQuestRequirements(const CatchChallenger::Quest &quest);
    bool haveStartQuestRequirement(const CatchChallenger::Quest &quest);
    bool nextStepQuest(const Quest &quest);
    bool startQuest(const Quest &quest);
    void addQuestStepDrop(const uint16_t &questId,const uint8_t &questStep);
    void removeQuestStepDrop(const uint16_t &questId,const uint8_t &questStep);
    void syncDatabaseQuest();
    void cancelQuest(const uint16_t &questId);

    bool checkCollision();

    bool getBattleIsValidated();
    bool isInFight() const override;
    void saveCurrentMonsterStat();
    void saveMonsterStat(const PlayerMonster &monster);
    void syncMonsterSkillAndEndurance(const PlayerMonster &monster);
    void syncMonsterEndurance(const PlayerMonster &monster);
    void syncMonsterBuff(const PlayerMonster &monster);
    bool checkLoose(bool withTeleport=true);
    bool isInBattle() const override;
    bool learnSkillInternal(const uint8_t &monsterPosition,const uint16_t &skill);
    void getRandomNumberIfNeeded() const;
    bool botFightCollision(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y);
    bool checkFightCollision(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y);
    void registerBattleRequest(Client * otherPlayerBattle);
    void saveAllMonsterPosition();

    void battleFinished();
    void battleFinishedReset();
    Client * getOtherPlayerBattle() const;
    bool finishTheTurn(const bool &isBot);
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    void displayCurrentStatus();
    #endif
    bool useSkill(const uint16_t &skill) override;
    bool currentMonsterAttackFirst(const PlayerMonster * currentMonster,const PublicPlayerMonster * otherMonster) const override;
    void healAllMonsters() override;
    void battleFakeAccepted(Client * otherPlayer);
    void battleFakeAcceptedInternal(Client *otherPlayer);
    bool botFightStart(const uint16_t &botFightId);
    int addCurrentBuffEffect(const Skill::BuffEffect &effect) override;
    bool moveUpMonster(const uint8_t &number) override;
    bool moveDownMonster(const uint8_t &number) override;
    void saveMonsterPosition(const uint32_t &monsterId,const uint8_t &monsterPosition);
    bool doTheOtherMonsterTurn() override;
    Skill::AttackReturn generateOtherAttack() override;
    Skill::AttackReturn doTheCurrentMonsterAttack(const uint16_t &skill, const uint8_t &skillLevel) override;
    uint8_t decreaseSkillEndurance(PlayerMonster::PlayerSkill * skill) override;
    void emitBattleWin();
    void hpChange(PlayerMonster * currentMonster, const uint32_t &newHpValue) override;
    bool removeBuffOnMonster(PlayerMonster * currentMonster, const uint32_t &buffId) override;
    bool removeAllBuffOnMonster(PlayerMonster * currentMonster) override;
    bool addLevel(PlayerMonster * monster, const uint8_t &numberOfLevel=1) override;

    bool checkKOCurrentMonsters();
    void syncForEndOfTurn();
    void saveStat();
    bool buffIsValid(const Skill::BuffEffect &buffEffect);
    bool haveBattleAction() const;
    void resetBattleAction();
    uint8_t getOtherSelectedMonsterNumber() const;
    void haveUsedTheBattleAction();
    void sendBattleReturn();
    void sendBattleMonsterChange();
    uint8_t selectedMonsterNumberToMonsterPlace(const uint8_t &selectedMonsterNumber);
    void internalBattleCanceled(const bool &send);
    void internalBattleAccepted(const bool &send);
    void resetTheBattle();
    PublicPlayerMonster *getOtherMonster() override;
    void fightFinished() override;
    bool giveXPSP(int xp,int sp) override;
    bool useSkillAgainstBotMonster(const uint16_t &skill, const uint8_t &skillLevel);
    void wildDrop(const uint16_t &monster) override;
    uint8_t getOneSeed(const uint8_t &max) override;
    bool bothRealPlayerIsReady() const;
    bool checkIfCanDoTheTurn();
    bool dropKOOtherMonster() override;
    uint32_t catchAWild(const bool &toStorage, const PlayerMonster &newMonster) override;
    bool haveCurrentSkill() const;
    uint16_t getCurrentSkill() const;
    bool haveMonsterChange() const;
    bool addSkill(PlayerMonster * currentMonster,const PlayerMonster::PlayerSkill &skill) override;
    bool setSkillLevel(PlayerMonster * currentMonster,const unsigned int &index,const uint8_t &level) override;
    bool removeSkill(PlayerMonster * currentMonster,const unsigned int &index) override;

    //trade
    Client * otherPlayerTrade;
    bool tradeIsValidated;
    bool tradeIsFreezed;
    uint64_t tradeCash;
    std::unordered_map<uint16_t,uint32_t> tradeObjects;
    std::vector<PlayerMonster> tradeMonster;
    std::vector<uint32_t> inviteToClanList;
    Clan *clan;
    Client * otherCityPlayerBattle;
public:
    #ifdef EPOLLCATCHCHALLENGERSERVER
    char *socketString;
    int socketStringSize;
    #endif

private:
    //trade
    void internalTradeCanceled(const bool &send);
    void internalTradeAccepted(const bool &send);
    //other
    static MonsterDrops questItemMonsterToMonsterDrops(const Quest::ItemMonster &questItemMonster);
    bool otherPlayerIsInRange(Client * otherPlayer);

    static uint32_t getMonsterId(bool * const ok);
    static uint32_t getClanId(bool * const ok);
    bool getInTrade() const;
    void registerTradeRequest(Client * otherPlayerTrade);
    bool getIsFreezed() const;
    uint64_t getTradeCash() const;
    std::unordered_map<uint16_t, uint32_t> getTradeObjects() const;
    std::vector<PlayerMonster> getTradeMonster() const;
    void resetTheTrade();
    void transferExistingMonster(std::vector<PlayerMonster> tradeMonster);
    PlayerMonster &getSelectedMonster();
    uint8_t getSelectedMonsterNumber();
    PlayerMonster& getEnemyMonster();
    void dissolvedClan();
    bool inviteToClan(const uint32_t &clanId);
    void insertIntoAClan(const uint32_t &clanId);
    void ejectToClan();

    void insertClientOnMap(CommonMap *map);
    void removeClientOnMap(CommonMap *map);

    void errorFightEngine(const std::string &error) override;
    void messageFightEngine(const std::string &message) const override;

    struct PlantInWaiting
    {
        uint8_t plant_id;
        CommonMap *map;
        uint8_t x,y;
    };
    std::queue<PlantInWaiting> plant_list_in_waiting;

    bool parseInputBeforeLogin(const uint8_t &packetCode, const uint8_t &queryNumber, const char * const data,const unsigned int &size);
    //have message without reply
    bool parseMessage(const uint8_t &packetCode,const char * const data,const unsigned int &size) override;
    //have query with reply
    bool parseQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size) override;
    //send reply
    bool parseReplyData(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size) override;

    #ifdef EPOLLCATCHCHALLENGERSERVER
    #if defined(CATCHCHALLENGER_CLASS_ALLINONESERVER) || defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER)
    void moveClientFastPath(const uint8_t &previousMovedUnit,const uint8_t &direction) override;
    #else
    void moveClientFastPath(const uint8_t &previousMovedUnit,const uint8_t &direction);
    #endif
    #else
    void moveClientFastPath(const uint8_t &previousMovedUnit,const uint8_t &direction);
    #endif

    // ------------------------------
    bool sendFile(const std::string &datapackPath,const std::string &fileName);

    void characterIsRight(const uint8_t &query_id, uint32_t characterId, CommonMap* map, const /*COORD_TYPE*/ uint8_t &x, const /*COORD_TYPE*/ uint8_t &y, const Orientation &orientation);
    void characterIsRightWithParsedRescue(const uint8_t &query_id, uint32_t characterId, CommonMap* map, const /*COORD_TYPE*/ uint8_t &x, const /*COORD_TYPE*/ uint8_t &y, const Orientation &orientation,
                      CommonMap* rescue_map, const /*COORD_TYPE*/ uint8_t &rescue_x, const /*COORD_TYPE*/ uint8_t &rescue_y, const Orientation &rescue_orientation,
                      CommonMap* unvalidated_rescue_map, const /*COORD_TYPE*/ uint8_t &unvalidated_rescue_x, const /*COORD_TYPE*/ uint8_t &unvalidated_rescue_y, const Orientation &unvalidated_rescue_orientation
                      );
    void characterIsRightWithRescue(const uint8_t &query_id,uint32_t characterId,CommonMap* map,const /*COORD_TYPE*/ uint8_t &x,const /*COORD_TYPE*/ uint8_t &y,const Orientation &orientation,
                      const std::string &rescue_map,const std::string &rescue_x,const std::string &rescue_y,const std::string &rescue_orientation,
                      const std::string &unvalidated_rescue_map,const std::string &unvalidated_rescue_x,const std::string &unvalidated_rescue_y,const std::string &unvalidated_rescue_orientation
                      );
    void characterIsRightFinalStep();
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    void loginIsWrong(const uint8_t &query_id,const uint8_t &returnCode,const std::string &debugMessage);
    void askStatClient(const uint8_t &query_id,const char *rawdata);
    #endif
    void characterSelectionIsWrong(const uint8_t &query_id,const uint8_t &returnCode,const std::string &debugMessage);
    //load linked data (like item, quests, ...)
    //void loadLinkedData();

    void loadMonsters();
    static void loadMonsters_static(void *object);
    void loadMonsters_return();

    static PlayerMonster loadMonsters_DatabaseReturn_to_PlayerMonster(bool &ok);
    static bool loadBuffBlock(const std::string &dataHexa,PlayerMonster &playerMonster);
    static bool loadSkillBlock(const std::string &dataHexa,PlayerMonster &playerMonster);
    static bool loadSkillEnduranceBlock(const std::string &dataHexa,PlayerMonster &playerMonster);

    /*void loadItems();
    void loadItemsWarehouse();
    void loadRecipes();
    void loadReputation();
    void loadQuests();
    void loadBotAlreadyBeaten();
    void loadPlayerMonsterBuffs(const uint32_t &index);
    void loadPlayerMonsterSkills(const uint32_t &index);
    void loadPlayerAllow();

    static void loadItems_static(void *object);
    void loadItems_return();
    static void loadItemsWarehouse_static(void *object);
    void loadItemsWarehouse_return();
    static void loadRecipes_static(void *object);
    void loadRecipes_return();
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
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    void loadPlantOnMap();
    static void loadPlantOnMap_static(void *object);
    void loadPlantOnMap_return();
    #endif

    static void loadPlayerMonsterBuffs_static(void *object);
    void loadPlayerMonsterBuffs_object();
    void loadPlayerMonsterBuffs_return(const uint32_t &index);
    static void loadPlayerMonsterSkills_static(void *object);
    void loadPlayerMonsterSkills_object();
    void loadPlayerMonsterSkills_return(const uint32_t &index);
*/

    uint32_t tryCapture(const uint16_t &item) override;
    bool changeOfMonsterInFight(const uint8_t &monsterPosition) override;
    void confirmEvolutionTo(PlayerMonster * playerMonster,const uint16_t &monster) override;
    std::vector<uint8_t> addPlayerMonster(const std::vector<PlayerMonster> &playerMonster) override;
    std::vector<uint8_t> addPlayerMonster(const PlayerMonster &playerMonster) override;
    bool addPlayerMonsterWithChange(const PlayerMonster &playerMonster);
    bool addToEncyclopedia(const uint16_t &monster);

    bool sendInventory();

    void generateRandomNumber();
    uint32_t randomSeedsSize() const override;
protected:
    //normal management related
    void errorOutput(const std::string &errorString) override;
    void kick();
    void normalOutput(const std::string &message) const override;
    std::string headerOutput() const;
    //drop all clients
    void dropAllClients();
    void dropAllBorderClients();
    //input/ouput layer
    void errorParsingLayer(const std::string &error) override;
    void messageParsingLayer(const std::string &message) const override;
    //map move
    virtual bool singleMove(const Direction &direction) override;
    virtual void put_on_the_map(CommonMap *map,const /*COORD_TYPE*/uint8_t &x,const /*COORD_TYPE*/uint8_t &y,const Orientation &orientation) override;
    virtual void teleportValidatedTo(CommonMap *map,const /*COORD_TYPE*/uint8_t &x,const /*COORD_TYPE*/uint8_t &y,const Orientation &orientation) override;
    virtual bool moveThePlayer(const uint8_t &previousMovedUnit,const Direction &direction) override;
    virtual void extraStop() = 0;
};
}

#endif // CLIENT_H
