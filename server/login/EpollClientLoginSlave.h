#ifndef EPOLLCLIENTLOGINMASTER_H
#define EPOLLCLIENTLOGINMASTER_H

#include "../epoll/EpollClient.h"
#include "../epoll/EpollSslClient.h"
#include "../../general/base/ProtocolParsing.h"
#include "../VariableServer.h"
#include "../epoll/db/EpollPostgresql.h"
#include "LinkToMaster.h"
#include "LinkToGameServer.h"

#include <string>
#include <vector>
#include <map>
#include <queue>

#define BASE_PROTOCOL_MAGIC_SIZE 8

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

#define CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE 8
#define CATCHCHALLENGER_DDOS_COMPUTERINTERVAL 5
#define CATCHCHALLENGER_DDOS_KICKLIMITMOVE 60
#define CATCHCHALLENGER_DDOS_KICKLIMITCHAT 5
#define CATCHCHALLENGER_DDOS_KICKLIMITOTHER 30

namespace CatchChallenger {
class EpollClientLoginSlave : public BaseClassSwitch, public ProtocolParsingInputOutput
{
public:
    EpollClientLoginSlave(
        #ifdef SERVERSSL
            const int &infd, SSL_CTX *ctx
        #else
            const int &infd
        #endif
        );
    ~EpollClientLoginSlave();
    struct AddCharacterParam
    {
        uint8_t query_id;
        uint8_t profileIndex;
        std::string pseudo;
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
    enum ProxyMode
    {
        Reconnect=0x01,
        Proxy=0x02
    };
    static ProxyMode proxyMode;
    enum EpollClientLoginStat
    {
        None,
        ProtocolGood,
        LoginInProgress,
        Logged,
        CharacterSelecting,
        CharacterSelected,
        GameServerConnecting,
        GameServerConnected,
    };
    EpollClientLoginStat stat;

    void parseIncommingData();
    void doDDOSCompute();
    static void doDDOSComputeAll();

    void selectCharacter_ReturnToken(const uint8_t &query_id,const char * const token);
    void selectCharacter_ReturnFailed(const uint8_t &query_id,const uint8_t &errorCode);
    void addCharacter_ReturnOk(const uint8_t &query_id,const uint32_t &characterId);
    void addCharacter_ReturnFailed(const uint8_t &query_id,const uint8_t &errorCode);
    void removeCharacter_ReturnOk(const uint8_t &query_id);
    void removeCharacter_ReturnFailed(const uint8_t &query_id,const uint8_t &errorCode,const std::string &errorString=std::string());
    bool removeFromQueryReceived(const uint8_t &queryNumber);
    void parseNetworkReadError(const std::string &errorString);

    LinkToGameServer *linkToGameServer;
    uint8_t charactersGroupIndex;
    uint32_t serverUniqueKey;
    char *socketString;
    int socketStringSize;
    unsigned int account_id;
    /** \warning Need be checked in real time because can be opened on multiple login server
     * uint8_t accountCharatersCount; */

    static std::vector<unsigned int> maxAccountIdList;
    static std::vector<unsigned int> maxCharacterIdList;
    static std::vector<unsigned int> maxClanIdList;
    static bool maxAccountIdRequested;
    static bool maxCharacterIdRequested;
    static bool maxMonsterIdRequested;
    static char replyToRegisterLoginServerCharactersGroup[1024];
    static unsigned int replyToRegisterLoginServerCharactersGroupSize;
    static char baseDatapackSum[28];
    static char loginGood[256*1024];
    static unsigned int loginGoodSize;

    static char serverServerList[256*1024];
    static unsigned int serverServerListSize;
    static unsigned int serverServerListCurrentPlayerSize;
    static char serverServerListComputedMessage[256*1024];
    static unsigned int serverServerListComputedMessageSize;
    static char serverLogicalGroupList[256*1024];
    static unsigned int serverLogicalGroupListSize;
    static char serverLogicalGroupAndServerList[512*1024];
    static unsigned int serverLogicalGroupAndServerListSize;

    static unsigned char loginIsWrongBufferReply[4];
private:
    std::queue<DatabaseBase::CallBack *> callbackRegistred;
    std::queue<void *> paramToPassToCallBack;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    std::queue<std::string> paramToPassToCallBackType;
    #endif

    static unsigned char protocolReplyProtocolNotSupported[4];
    static unsigned char protocolReplyServerFull[4];
    static unsigned char protocolReplyCompressionNone[4+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    static unsigned char protocolReplyCompresssionZlib[4+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    static unsigned char protocolReplyCompressionXz[4+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];

    static unsigned char loginInProgressBuffer[4];
    static unsigned char addCharacterIsWrongBuffer[8];
    static char loginCharacterList[1024];
    static unsigned char addCharacterReply[3+1+4];
    static unsigned char removeCharacterReply[3+1];

    static const unsigned char protocolHeaderToMatch[5];
    BaseClassSwitch::EpollObjectType getType() const;
private:
    void errorParsingLayer(const std::string &error);
    void messageParsingLayer(const std::string &message) const;
    void errorParsingLayer(const char * const error);
    void messageParsingLayer(const char * const message) const;
    //have message without reply
    bool parseMessage(const uint8_t &mainCodeType,const char * const data,const unsigned int &size);
    //have query with reply
    bool parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size);
    //send reply
    bool parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size);

    bool parseInputBeforeLogin(const uint8_t &mainCodeType, const uint8_t &queryNumber, const char * const data, const unsigned int &size);
    void disconnectClient();
public:
    void askLogin_cancel();
    void askLogin(const uint8_t &query_id,const char *rawdata);
    static void askLogin_static(void *object);
    void askLogin_object();
    void askLogin_return(AskLoginParam *askLoginParam);
    void createAccount(const uint8_t &query_id, const char *rawdata);
    static void createAccount_static(void *object);
    void createAccount_object();
    void createAccount_return(AskLoginParam *askLoginParam);

    void character_list_return(const uint8_t &characterGroupIndex,char * const tempRawData,const int &tempRawDataSize);
    void server_list_return(const uint8_t &serverCount,char * const tempRawData,const int &tempRawDataSize);

    bool sendRawBlock(const char * const data, const unsigned int &size);
private:
    void deleteCharacterNow(const uint32_t &characterId);
    void addCharacter(const uint8_t &query_id, const uint8_t &characterGroupIndex, const uint8_t &profileIndex, const std::string &pseudo, const uint8_t &skinId);
    void removeCharacter(const uint8_t &query_id, const uint8_t &characterGroupIndex, const uint32_t &characterId);
    void dbQueryWriteLogin(const std::string &queryText);

    void loginIsWrong(const uint8_t &query_id,const uint8_t &returnCode,const std::string &debugMessage);
    void selectCharacter(const uint8_t &query_id,const uint32_t &serverUniqueKey,const uint8_t &charactersGroupIndex,const uint32_t &characterId);
private:
    struct CharacterListForReply
    {
        char *rawData;
        int rawDataSize;
    };
    //to be ordered
    std::map<uint8_t,CharacterListForReply> characterTempListForReply;
    uint8_t characterListForReplyInSuspend;
    //by client because is where is merged all servers time reply from all catchchallenger_common*
    char *serverListForReplyRawData;
    unsigned int serverListForReplyRawDataSize;

    uint8_t serverListForReplyInSuspend;
    static std::vector<EpollClientLoginSlave *> client_list;

    uint8_t movePacketKick[CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE];
    uint8_t movePacketKickSize;
    uint8_t movePacketKickTotalCache;
    uint8_t movePacketKickNewValue;
    uint8_t chatPacketKick[CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE];
    uint8_t chatPacketKickSize;
    uint8_t chatPacketKickTotalCache;
    uint8_t chatPacketKickNewValue;
    uint8_t otherPacketKick[CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE];
    uint8_t otherPacketKickSize;
    uint8_t otherPacketKickTotalCache;
    uint8_t otherPacketKickNewValue;
public:
    static EpollPostgresql databaseBaseLogin;
    static EpollPostgresql databaseBaseCommon;
};
}

#endif // EPOLLCLIENTLOGINMASTER_H
