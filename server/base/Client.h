#ifndef CATCHCHALLENGER_CLIENT_H
#define CATCHCHALLENGER_CLIENT_H

#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QObject>
#include <QTimer>
#endif
#include <vector>

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
#include <unordered_map>
#include <vector>
#include <queue>
#include <iostream>

#ifdef EPOLLCATCHCHALLENGERSERVER
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

namespace CatchChallenger {

#ifdef EPOLLCATCHCHALLENGERSERVER
void recordDisconnectByServer(void * client);
#endif

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
    BaseClassSwitch::EpollObjectType getType() const;
    #endif
    //to get some info
    std::string getPseudo() const;
    void savePosition();
    static bool characterConnected(const uint32_t &characterId);
    void disconnectClient();
    static void disconnectClientById(const uint32_t &characterId);
    Client *getClientFight() const;
    #ifdef CATCHCHALLENGER_DDOS_FILTER
    void doDDOSCompute();
    #endif
    void receive_instant_player_number(const uint16_t &connected_players, const char * const data, const uint8_t &size);
    void parseIncommingData();
    #ifndef EPOLLCATCHCHALLENGERSERVER
    static void startTheCityCapture();
    #endif
    static void setEvent(const uint8_t &event, const uint8_t &new_value);
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    static char * addAuthGetToken(const uint32_t &characterId,const uint32_t &accountIdRequester);
    static void purgeTokenAuthList();
    #endif
    uint32_t getPlayerId() const;
    uint32_t getClanId() const;
    bool haveAClan() const;

    bool sendRawBlock(const char * const data,const unsigned int &size);

    static std::vector<int> generalChatDrop;
    static int generalChatDropTotalCache;
    static int generalChatDropNewValue;
    static std::vector<int> clanChatDrop;
    static int clanChatDropTotalCache;
    static int clanChatDropNewValue;
    static std::vector<int> privateChatDrop;
    static int privateChatDropTotalCache;
    static int privateChatDropNewValue;
    static std::vector<uint16_t> marketObjectIdList;
    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    static uint64_t datapack_list_cache_timestamp_base,datapack_list_cache_timestamp_main,datapack_list_cache_timestamp_sub;
    #endif
    static std::vector<uint16_t> simplifiedIdList;
    static std::unordered_map<std::string,Client *> playerByPseudo;
    static std::unordered_map<uint32_t,Clan *> clanList;
    static std::vector<Client *> clientBroadCastList;

    static unsigned char protocolReplyProtocolNotSupported[7];
    static unsigned char protocolReplyServerFull[7];
    static unsigned char *characterIsRightFinalStepHeader;
    static uint32_t characterIsRightFinalStepHeaderSize;
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    /// \todo group all reply in one
    static unsigned char protocolReplyCompressionNone[7+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    static unsigned char protocolReplyCompresssionZlib[7+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    static unsigned char protocolReplyCompressionXz[7+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    static unsigned char protocolReplyCompressionLz4[7+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    #endif
    static std::vector<Client *> stat_client_list;
    static unsigned char private_token_statclient[TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];

    static unsigned char *protocolReplyCharacterList;
    static uint16_t protocolReplyCharacterListSize;
    static unsigned char *protocolMessageLogicalGroupAndServerList;
    static uint16_t protocolMessageLogicalGroupAndServerListSize;
    static uint16_t protocolMessageLogicalGroupAndServerListPosPlayerNumber;
    #else
    static unsigned char protocolReplyCompressionNone[7];
    static unsigned char protocolReplyCompresssionZlib[7];
    static unsigned char protocolReplyCompressionXz[7];
    static unsigned char protocolReplyCompressionLz4[7];
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

    std::queue<CatchChallenger::DatabaseBase::CallBack *> callbackRegistred;

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
    uint8_t movePacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE];
    uint8_t movePacketKickSize;
    uint8_t movePacketKickTotalCache;
    uint8_t movePacketKickNewValue;
    uint8_t chatPacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE];
    uint8_t chatPacketKickSize;
    uint8_t chatPacketKickTotalCache;
    uint8_t chatPacketKickNewValue;
    uint8_t otherPacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE];
    uint8_t otherPacketKickSize;
    uint8_t otherPacketKickTotalCache;
    uint8_t otherPacketKickNewValue;
    #endif
    uint8_t profileIndex;
    std::queue<PlayerOnMap> lastTeleportation;
    std::vector<uint8_t> queryNumberList;

    Client *otherPlayerBattle;
    bool battleIsValidated;
    uint32_t mCurrentSkillId;
    bool mHaveCurrentSkill,mMonsterChange;
    uint32_t botFightCash;
    uint32_t botFightId;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    bool isInCityCapture;
    #endif
    std::vector<Skill::AttackReturn> attackReturn;
    //std::unordered_map<uint32_t/*currentMonster->id*/, std::unordered_map<uint32_t/*skill*/,uint32_t> > deferedEnduranceSync;
    std::unordered_set<PlayerMonster *> deferedEnduranceSync;

    //player indexed list
    static const std::string text_chat;
    static const std::string text_space;
    static const std::string text_system;
    static const std::string text_system_important;
    static const std::string text_setrights;
    static const std::string text_normal;
    static const std::string text_premium;
    static const std::string text_gm;
    static const std::string text_dev;
    static const std::string text_playerlist;
    static const std::string text_startbold;
    static const std::string text_stopbold;
    static const std::string text_playernumber;
    static const std::string text_kick;
    static const std::string text_Youarealoneontheserver;
    static const std::string text_playersconnected;
    static const std::string text_playersconnectedspace;
    static const std::string text_havebeenkickedby;
    static const std::string text_unknowcommand;
    static const std::string text_commandnotunderstand;
    static const std::string text_command;
    static const std::string text_commaspace;
    static const std::string text_unabletofoundtheconnectedplayertokick;
    static const std::string text_unabletofoundthisrightslevel;

    static const std::string text_server_full;
    static const std::string text_slashpmspace;
    static const std::string text_slash;
    static const std::string text_regexresult1;
    static const std::string text_regexresult2;
    static const std::string text_send_command_slash;
    static const std::string text_trade;
    static const std::string text_battle;
    static const std::string text_give;
    static const std::string text_setevent;
    static const std::string text_take;
    static const std::string text_tp;
    static const std::string text_stop;
    static const std::string text_restart;
    static const std::string text_unknown_send_command_slash;
    static const std::string text_commands_seem_not_right;

    struct DatapackCacheFile
    {
        //uint32_t mtime;
        uint32_t partialHash;
    };
    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    static uint8_t tempDatapackListReplySize;
    static std::vector<char> tempDatapackListReplyArray;
    static uint8_t tempDatapackListReply;
    static unsigned int tempDatapackListReplyTestCount;
    //static std::unordered_map<std::string,uint32_t> datapack_file_list_cache_base,datapack_file_list_cache_main,datapack_file_list_cache_sub;//same than above
    static std::unordered_map<std::string,DatapackCacheFile> datapack_file_hash_cache_base,datapack_file_hash_cache_main,datapack_file_hash_cache_sub;
    #endif
    static std::regex fileNameStartStringRegex;

    static std::string single_quote;
    static std::string antislash_single_quote;
    static const std::string text_dotslash;
    static const std::string text_dotcomma;
    static const std::string text_double_slash;
    static const std::string text_antislash;
    static const std::string text_top;
    static const std::string text_bottom;
    static const std::string text_left;
    static const std::string text_right;
    static const std::string text_dottmx;
    static const std::string text_unknown;
    static const std::string text_female;
    static const std::string text_male;
    static const std::string text_warehouse;
    static const std::string text_wear;
    static const std::string text_market;

    //info linked
    static Direction	temp_direction;
    static std::unordered_map<uint32_t,Client *> playerById;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    static std::unordered_map<std::string,std::vector<Client *> > captureCity;
    static std::unordered_map<std::string,CaptureCityValidated> captureCityValidatedList;
    #endif
    static std::unordered_map<uint32_t,uint64_t> characterCreationDateList;

    static const std::string text_0;
    static const std::string text_1;
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    static const std::string text_2;
    #endif
    static const std::string text_false;
    static const std::string text_true;
    static const std::string text_to;

    //socket related
    #ifndef EPOLLCATCHCHALLENGERSERVER
    void connectionError(QAbstractSocket::SocketError);
    #endif

    #ifndef EPOLLCATCHCHALLENGERSERVER
    /// \warning it need be complete protocol trame
    void fake_receive_data(std::vector<char> data);
    #endif
    //global slot
    void sendPM(const std::string &text,const std::string &pseudo);
    void receiveChatText(const Chat_type &chatType, const std::string &text, const Client *sender_informations);
    void receiveSystemText(const std::string &text,const bool &important=false);
    void sendChatText(const Chat_type &chatType,const std::string &text);
    void sendBroadCastCommand(const std::string &command,const std::string &extraText);
    void setRights(const Player_type& type);
    //normal slots
    void sendSystemMessage(const std::string &text,const bool &important=false);
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
    static std::unordered_map<std::string,DatapackCacheFile> datapack_file_list(const std::string &path,const std::string &exclude,const bool withHash=true);//used into BaseServer to do the hash
    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    //check each element of the datapack, determine if need be removed, updated, add as new file all the missing file
    void datapackList(const uint8_t &query_id, const std::vector<std::string > &files, const std::vector<uint32_t> &partialHashList);
    std::unordered_map<std::string,Client::DatapackCacheFile> datapack_file_list_cached_base();
    std::unordered_map<std::string,Client::DatapackCacheFile> datapack_file_list_cached_main();
    std::unordered_map<std::string,Client::DatapackCacheFile> datapack_file_list_cached_sub();
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
    void plantSeed(
        #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        const uint8_t &query_id,
        #endif
        const uint8_t &plant_id);
    void collectPlant(
            #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
            const uint8_t &query_id
            #endif
            );

    void createMemoryClan();
    Direction lookToMove(const Direction &direction);
    //seed
    void useSeed(const uint8_t &plant_id);
    //crafting
    void useRecipe(const uint8_t &query_id,const uint32_t &recipe_id);
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
    void wareHouseStore(const int64_t &cash, const std::vector<std::pair<uint16_t, int32_t> > &items, const std::vector<uint32_t> &withdrawMonsters, const std::vector<uint32_t> &depositeMonsters);
    bool wareHouseStoreCheck(const int64_t &cash, const std::vector<std::pair<uint16_t, int32_t> > &items, const std::vector<uint32_t> &withdrawMonsters, const std::vector<uint32_t> &depositeMonsters);
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
    bool useObjectOnMonsterByPosition(const uint16_t &object, const uint8_t &monsterPosition);
    //teleportation
    void receiveTeleportTo(CommonMap *map,const /*COORD_TYPE*/uint8_t &x,const /*COORD_TYPE*/uint8_t &y,const Orientation &orientation);
    //shop
    void getShopList(const uint8_t &query_id, const uint16_t &shopId);
    void buyObject(const uint8_t &query_id, const uint16_t &shopId, const uint16_t &objectId, const uint32_t &quantity, const uint32_t &price);
    void sellObject(const uint8_t &query_id,const uint16_t &shopId,const uint16_t &objectId,const uint32_t &quantity,const uint32_t &price);
    //factory
    void saveIndustryStatus(const uint32_t &factoryId,const IndustryStatus &industryStatus,const Industry &industry);
    void getFactoryList(const uint8_t &query_id, const uint16_t &factoryId);
    void buyFactoryProduct(const uint8_t &query_id,const uint16_t &factoryId,const uint16_t &objectId,const uint32_t &quantity,const uint32_t &price);
    void sellFactoryResource(const uint8_t &query_id,const uint16_t &factoryId,const uint16_t &objectId,const uint32_t &quantity,const uint32_t &price);
    //trade
    void tradeCanceled();
    void tradeAccepted();
    void tradeFinished();
    void tradeAddTradeCash(const uint64_t &cash);
    void tradeAddTradeObject(const uint16_t &item,const uint32_t &quantity);
    void tradeAddTradeMonster(const uint8_t &monsterPosition);
    //quest
    void newQuestAction(const QuestAction &action,const uint32_t &questId);
    void appendAllow(const ActionAllow &allow);
    void removeAllow(const ActionAllow &allow);
    void syncDatabaseAllow();
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    //plant
    bool syncDatabasePlant();
    #endif
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
    virtual bool tryEscape();
    void heal();
    void requestFight(const uint16_t &fightId);
    bool learnSkill(const uint8_t &monsterPosition,const uint16_t &skill);
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
    #ifndef EPOLLCATCHCHALLENGERSERVER
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
    #endif
    void fightOrBattleFinish(const bool &win,const uint32_t &fightId);//fightId == 0 if is in battle
    void moveMonster(const bool &up,const uint8_t &number);
    //market
    void getMarketList(const uint32_t &query_id);
    void buyMarketObject(const uint32_t &query_id,const uint32_t &marketObjectId,const uint32_t &quantity);
    void buyMarketMonster(const uint32_t &query_id, const uint32_t &marketMonsterUniqueId/*To ident even if the position have changed, imply search at server*/);
    void putMarketObject(const uint32_t &query_id, const uint32_t &objectId, const uint32_t &quantity, const uint64_t &price);
    void putMarketMonster(const uint32_t &query_id, const uint8_t &monsterPosition, const uint64_t &price);
    void withdrawMarketCash(const uint32_t &query_id);
    void withdrawMarketObject(const uint32_t &query_id,const uint32_t &objectId,const uint32_t &quantity);
    void withdrawMarketMonster(const uint32_t &query_id, const uint32_t &marketMonsterUniqueId/*To ident even if the position have changed, imply search at server*/);

    static std::string directionToStringToSave(const Direction &direction);
    static std::string orientationToStringToSave(const Orientation &orientation);
    //quest
    bool haveNextStepQuestRequirements(const CatchChallenger::Quest &quest);
    bool haveStartQuestRequirement(const CatchChallenger::Quest &quest);
    bool nextStepQuest(const Quest &quest);
    bool startQuest(const Quest &quest);
    void addQuestStepDrop(const uint32_t &questId,const uint8_t &questStep);
    void removeQuestStepDrop(const uint32_t &questId,const uint8_t &questStep);
    void syncDatabaseQuest();

    bool checkCollision();

    bool getBattleIsValidated();
    bool isInFight() const;
    void saveCurrentMonsterStat();
    void saveMonsterStat(const PlayerMonster &monster);
    void syncMonsterSkillAndEndurance(const PlayerMonster &monster);
    void syncMonsterEndurance(const PlayerMonster &monster);
    void syncMonsterBuff(const PlayerMonster &monster);
    bool checkLoose(bool withTeleport=true);
    bool isInBattle() const;
    bool learnSkillInternal(const uint8_t &monsterPosition,const uint32_t &skill);
    void getRandomNumberIfNeeded() const;
    bool botFightCollision(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y);
    bool checkFightCollision(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y);
    void registerBattleRequest(Client * otherPlayerBattle);
    void saveAllMonsterPosition();
    bool haveBeatBot(const uint16_t &botFightId) const;

    void battleFinished();
    void battleFinishedReset();
    Client * getOtherPlayerBattle() const;
    bool finishTheTurn(const bool &isBot);
    bool useSkill(const uint32_t &skill);
    bool currentMonsterAttackFirst(const PlayerMonster * currentMonster,const PublicPlayerMonster * otherMonster) const;
    void healAllMonsters();
    void battleFakeAccepted(Client * otherPlayer);
    void battleFakeAcceptedInternal(Client *otherPlayer);
    bool botFightStart(const uint32_t &botFightId);
    int addCurrentBuffEffect(const Skill::BuffEffect &effect);
    bool moveUpMonster(const uint8_t &number);
    bool moveDownMonster(const uint8_t &number);
    void saveMonsterPosition(const uint32_t &monsterId,const uint8_t &monsterPosition);
    bool doTheOtherMonsterTurn();
    Skill::AttackReturn generateOtherAttack();
    Skill::AttackReturn doTheCurrentMonsterAttack(const uint32_t &skill, const uint8_t &skillLevel);
    uint8_t decreaseSkillEndurance(const uint32_t &skill);
    void emitBattleWin();
    void hpChange(PlayerMonster * currentMonster, const uint32_t &newHpValue);
    bool removeBuffOnMonster(PlayerMonster * currentMonster, const uint32_t &buffId);
    bool removeAllBuffOnMonster(PlayerMonster * currentMonster);
    bool addLevel(PlayerMonster * monster, const uint8_t &numberOfLevel=1);

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
    PublicPlayerMonster *getOtherMonster();
    void fightFinished();
    bool giveXPSP(int xp,int sp);
    bool useSkillAgainstBotMonster(const uint32_t &skill, const uint8_t &skillLevel);
    void wildDrop(const uint32_t &monster);
    uint8_t getOneSeed(const uint8_t &max);
    bool bothRealPlayerIsReady() const;
    bool checkIfCanDoTheTurn();
    bool dropKOOtherMonster();
    uint32_t catchAWild(const bool &toStorage, const PlayerMonster &newMonster);
    bool haveCurrentSkill() const;
    uint32_t getCurrentSkill() const;
    bool haveMonsterChange() const;
    bool addSkill(PlayerMonster * currentMonster,const PlayerMonster::PlayerSkill &skill);
    bool setSkillLevel(PlayerMonster * currentMonster,const unsigned int &index,const uint8_t &level);
    bool removeSkill(PlayerMonster * currentMonster,const unsigned int &index);

    //trade
    Client * otherPlayerTrade;
    bool tradeIsValidated;
    bool tradeIsFreezed;
    uint64_t tradeCash;
    std::unordered_map<uint32_t,uint32_t> tradeObjects;
    std::vector<PlayerMonster> tradeMonster;
    std::vector<uint32_t> inviteToClanList;
    Clan *clan;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    Client * otherCityPlayerBattle;
    #endif
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
    std::unordered_map<uint32_t,uint32_t> getTradeObjects() const;
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
    void removeClientOnMap(CommonMap *map
                                   #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
                                   , const bool &withDestroy=false
                                   #endif
                        );
    #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    void sendNearPlant();
    void removeNearPlant();
    #endif

    void errorFightEngine(const std::string &error);
    void messageFightEngine(const std::string &message) const;

    struct PlantInWaiting
    {
        #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        uint8_t query_id;
        #endif
        uint8_t plant_id;
        CommonMap *map;
        uint8_t x,y;
    };
    #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    static std::queue<PlantInWaiting> plant_list_in_waiting;
    #else
    std::queue<PlantInWaiting> plant_list_in_waiting;
    #endif

    bool parseInputBeforeLogin(const uint8_t &packetCode, const uint8_t &queryNumber, const char * const data,const unsigned int &size);
    //have message without reply
    bool parseMessage(const uint8_t &packetCode,const char * const data,const unsigned int &size);
    //have query with reply
    bool parseQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size);
    //send reply
    bool parseReplyData(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size);

    void moveClientFastPath(const uint8_t &previousMovedUnit,const uint8_t &direction);

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

    uint32_t tryCapture(const uint16_t &item);
    bool changeOfMonsterInFight(const uint8_t &monsterPosition);
    void confirmEvolutionTo(PlayerMonster * playerMonster,const uint32_t &monster);
    std::vector<uint8_t> addPlayerMonster(const std::vector<PlayerMonster> &playerMonster);
    std::vector<uint8_t> addPlayerMonster(const PlayerMonster &playerMonster);
    bool addPlayerMonsterWithChange(const PlayerMonster &playerMonster);
    bool addToEncyclopedia(const uint16_t &monster);

    bool sendInventory();

    void generateRandomNumber();
    uint32_t randomSeedsSize() const;
protected:
    //normal management related
    void errorOutput(const std::string &errorString);
    void kick();
    void normalOutput(const std::string &message) const;
    std::string headerOutput() const;
    //drop all clients
    void dropAllClients();
    void dropAllBorderClients();
    //input/ouput layer
    void errorParsingLayer(const std::string &error);
    void messageParsingLayer(const std::string &message) const;
    //map move
    virtual bool singleMove(const Direction &direction);
    virtual void put_on_the_map(CommonMap *map,const /*COORD_TYPE*/uint8_t &x,const /*COORD_TYPE*/uint8_t &y,const Orientation &orientation);
    virtual void teleportValidatedTo(CommonMap *map,const /*COORD_TYPE*/uint8_t &x,const /*COORD_TYPE*/uint8_t &y,const Orientation &orientation);
    virtual bool moveThePlayer(const uint8_t &previousMovedUnit,const Direction &direction);
    virtual void extraStop() = 0;
};
}

#endif // CLIENT_H
