#ifndef EPOLLCLIENTLOGINMASTER_H
#define EPOLLCLIENTLOGINMASTER_H

#include "../epoll/EpollClient.hpp"
#include "../../general/base/ProtocolParsing.hpp"
#include "../base/VariableServer.hpp"
#include "../epoll/db/EpollPostgresql.hpp"
#include "../base/DdosBuffer.hpp"

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
#define CATCHCHALLENGER_DDOS_KICKLIMITMOVE 140
#define CATCHCHALLENGER_DDOS_KICKLIMITCHAT 15
#define CATCHCHALLENGER_DDOS_KICKLIMITOTHER 45

namespace CatchChallenger {
class LinkToGameServer;
class EpollClientLoginSlave : public EpollClient, public ProtocolParsingInputOutput
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
    bool disconnectClient();
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
    enum ProxyMode : uint8_t
    {
        Reconnect=0x01,
        Proxy=0x02
    };
    static ProxyMode proxyMode;
    enum EpollClientLoginStat : uint8_t
    {
        None=0x00,
        ProtocolGood=0x01,
        LoginInProgress=0x02,
        Logged=0x03,
        LoggedStatClient=0x04,
        CharacterSelecting=0x05,
        CharacterSelected=0x06,
        GameServerConnecting=0x07,
        GameServerConnected=0x08 ,
    };
    EpollClientLoginStat stat;

    void parseIncommingData();
    void doDDOSCompute();
    static void doDDOSComputeAll();
    void breakNeedMoreData();

    void selectCharacter_ReturnToken(const uint8_t &query_id,const char * const token);
    /*errorCode: 02 (Character not found)
    03 (Character already connected/online or too recently disconnected)
    04 (Server internal problem)
    05 (Server not found)
    08 (Too recently disconnected)*/
    void selectCharacter_ReturnFailed(const uint8_t &query_id,const uint8_t &errorCode,const std::string &customError=std::string());
    void addCharacter_ReturnOk(const uint8_t &query_id,const uint32_t &characterId);
    void addCharacter_ReturnFailed(const uint8_t &query_id,const uint8_t &errorCode);
    void removeCharacter_ReturnOk(const uint8_t &query_id);
    void removeCharacter_ReturnFailed(const uint8_t &query_id,const uint8_t &errorCode,const std::string &errorString=std::string());
    bool removeFromQueryReceived(const uint8_t &queryNumber);
    void parseNetworkReadError(const std::string &errorString);
    void parseNetworkReadMessage(const std::string &errorString);

    uint64_t get_lastActivity() const;
    uint64_t lastActivity;
    LinkToGameServer *linkToGameServer;
    uint8_t charactersGroupIndex;
    uint32_t serverUniqueKey;
    char *socketString;
    int socketStringSize;
    unsigned int account_id;
    /** \warning Need be checked in real time because can be opened on multiple login server
     * uint8_t accountCharatersCount; */

    static std::vector<unsigned int> maxAccountIdList;
    static bool maxAccountIdRequested;
    static char replyToRegisterLoginServerCharactersGroup[1024];
    static unsigned int replyToRegisterLoginServerCharactersGroupSize;
    static char baseDatapackSum[28];
    static char loginGood[256*1024];
    static unsigned int loginGoodSize;
    static std::vector<EpollClientLoginSlave *> stat_client_list;

    /*  [0x00]: 8Bits: do into EpollServerLoginSlave::EpollServerLoginSlave(), login server mode: 0x01 reconnect, 0x02 proxy
        [0x01]: 8Bits: serverListSize
        All is stored into little endian to resend quickly
     *  if EpollClientLoginSlave::proxyMode==EpollClientLoginSlave::ProxyMode::Proxy
         * Loop (server list size: [0x00]):
         *  charactersgroup (8Bits)
         *  serverUniqueKey (32Bits)
         *  xmlStringSize (16Bits for size + variable size)
         *  logical group (8Bits)
         *  Max player (16Bits)
         * Loop (server list size: [0x00]):
         *  The current number of player 16Bits
     * else
         * Loop (server list size: [0x00]):
         *  charactersgroup (8Bits)
         *  serverUniqueKey (32Bits)
         *  host (8Bits for size + variable size)
         *  port (16Bits)
         *  xmlStringSize (16Bits for size + variable size)
         *  logical group (8Bits)
         *  Max player (16Bits)
         * Loop (server list size: [0x00]):
         *  The current number of player 16Bits
     *  */
    static char serverServerList[256*1024];
    static unsigned int serverServerListSize;//in bytes
    static unsigned int serverServerListCurrentPlayerSize;//in bytes
    /// \todo explain why not merged with serverServerList
    static char serverServerListComputedMessage[256*1024];//serverServerList with 0x40 header, and the rest of the header to have the packet prepared
    static unsigned int serverServerListComputedMessageSize;
    static char serverLogicalGroupList[256*1024];
    static unsigned int serverLogicalGroupListSize;
    static char serverLogicalGroupAndServerList[512*1024];
    static unsigned int serverLogicalGroupAndServerListSize;

    static unsigned char loginIsWrongBufferReply[1+1+4+1];
private:
    std::queue<DatabaseBaseCallBack *> callbackRegistred;
    std::queue<void *> paramToPassToCallBack;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    std::queue<std::string> paramToPassToCallBackType;
    #endif

    static unsigned char protocolReplyProtocolNotSupported[7];
    static unsigned char protocolReplyServerFull[7];
    /// \todo group all reply in one
    static unsigned char protocolReplyCompressionNone[7+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    static unsigned char protocolReplyCompresssionZstandard[7+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    #endif

    static unsigned char loginInProgressBuffer[7];
    static unsigned char addCharacterIsWrongBuffer[3+4];
    static char loginCharacterList[1024];
    static unsigned char addCharacterReply[3+4];
    static unsigned char removeCharacterReply[3+4];

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
public:
    void askLogin_cancel();
    void askLogin(const uint8_t &query_id,const char *rawdata);
    void askStatClient(const uint8_t &query_id,const char *rawdata);
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

    ssize_t read(char * data, const size_t &size);
    ssize_t write(const char * const data, const size_t &size);
    void closeSocket();
private:
    void deleteCharacterNow(const uint32_t &characterId);
    void addCharacter(const uint8_t &query_id, const uint8_t &characterGroupIndex, const uint8_t &profileIndex, const std::string &pseudo, const uint8_t &monsterGroupId, const uint8_t &skinId);
    void removeCharacterLater(const uint8_t &query_id, const uint8_t &characterGroupIndex, const uint32_t &characterId);
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

    DdosBuffer<uint8_t,3> movePacketKick;
    DdosBuffer<uint8_t,3> chatPacketKick;
    DdosBuffer<uint8_t,3> otherPacketKick;
public:
    static EpollPostgresql databaseBaseLogin;
    static EpollPostgresql databaseBaseCommon;
};
}

#endif // EPOLLCLIENTLOGINMASTER_H
