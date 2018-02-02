#ifndef EPOLLCLIENTLOGINMASTER_H
#define EPOLLCLIENTLOGINMASTER_H

#include "../../general/base/ProtocolParsing.h"
#include "CharactersGroup.h"
#include "../VariableServer.h"

#include <random>
#include <string>
#include <regex>

#define BASE_PROTOCOL_MAGIC_SIZE 9

namespace CatchChallenger {
class EpollClientLoginMaster : public BaseClassSwitch, public ProtocolParsingInputOutput
{
public:
    EpollClientLoginMaster(
        #ifdef SERVERSSL
            const int &infd, SSL_CTX *ctx
        #else
            const int &infd
        #endif
        );
    ~EpollClientLoginMaster();
    void disconnectClient();
    void parseIncommingData();
    void breakNeedMoreData();
    void selectCharacter(const uint8_t &query_id, const uint32_t &serverUniqueKey, const uint8_t &charactersGroupIndex, const uint32_t &characterId, const uint32_t &accountId);
    bool trySelectCharacterGameServer(EpollClientLoginMaster * const loginServer,const uint8_t &client_query_id,const uint32_t &serverUniqueKey,const uint8_t &charactersGroupIndex,const uint32_t &characterId, const uint32_t &accountId);
    void selectCharacter_ReturnToken(const uint8_t &query_id,const char * const token);
    void selectCharacter_ReturnFailed(const uint8_t &query_id, const uint8_t &errorCode);
    void disconnectForDuplicateConnexionDetected(const uint32_t &characterId);
    static void broadcastGameServerChange();
    static void sendServerChange();
    void addToRemoveList();
    void addToInserList();
    bool sendGameServerRegistrationReply(const uint8_t queryNumber,bool generateNewUniqueKey);
    bool sendGameServerPing(const uint64_t &msecondFrom1970);
    bool sendRawBlock(const char * const data,const int &size);
    void passUniqueKeyToNextGameServer();
    void resetToDisconnect();
    enum EpollClientLoginMasterStat : std::uint8_t
    {
        None,
        Logged,
        GameServer,
        LoginServer,
    };

    static uint32_t lastSizeDisplayLoginServers;
    static uint32_t lastSizeDisplayGameServers;
    static bool currentPlayerForGameServerToUpdate;
    EpollClientLoginMasterStat stat;
    std::mt19937 rng;
    std::string socketString;
    uint32_t uniqueKey;
    CharactersGroup *charactersGroupForGameServer;
    CharactersGroup::InternalGameServer *charactersGroupForGameServerInformation;

    EpollClientLoginMaster * inConflicWithTheMainServer;
    std::vector<EpollClientLoginMaster *> secondServerInConflict;
    uint8_t queryNumberInConflicWithTheMainServer;

    struct DataForSelectedCharacterReturn
    {
        EpollClientLoginMaster * loginServer;
        uint8_t client_query_id;
        uint32_t serverUniqueKey;
        uint8_t charactersGroupIndex;
        uint32_t characterId;
    };
    //to unordered reply
    //std::unordered_map<uint8_t,DataForSelectedCharacterReturn> loginServerReturnForCharaterSelect;
    //to ordered reply
    std::vector<DataForSelectedCharacterReturn> loginServerReturnForCharaterSelect;
    std::vector<uint8_t> queryNumberList;
    std::vector<char> tokenForAuth;

    static char private_token[TOKEN_SIZE_FOR_MASTERAUTH];
    static const unsigned char protocolHeaderToMatch[BASE_PROTOCOL_MAGIC_SIZE];
    static unsigned char protocolReplyProtocolNotSupported[7];
    static unsigned char protocolReplyWrongAuth[7];
    /// \todo group all reply in one
    static unsigned char protocolReplyCompressionNone[7+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    static unsigned char protocolReplyCompresssionZlib[7+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    static unsigned char protocolReplyCompressionXz[7+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    static unsigned char protocolReplyCompressionLz4[7+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    #endif
    static unsigned char selectCharaterRequestOnGameServer[2/*header*/+CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER];
    static unsigned char duplicateConnexionDetected[1/*header*/+4];
    static unsigned char getTokenForCharacterSelect[2/*header*/+4+4];
    static char loginSettingsAndCharactersGroup[256*1024];
    static unsigned int loginSettingsAndCharactersGroupSize;
    static char serverPartialServerList[256*1024];
    static unsigned int serverPartialServerListSize;
    static char serverServerList[256*1024];
    static unsigned int serverServerListSize;
    static char serverLogicalGroupList[256*1024];
    static unsigned int serverLogicalGroupListSize;
    static char loginPreviousToReplyCache[256*1024*3];
    static unsigned int loginPreviousToReplyCacheSize;
    static std::unordered_map<std::string,int> logicalGroupHash;

    static FILE *fpRandomFile;
    static std::vector<EpollClientLoginMaster *> gameServers;
    static std::vector<EpollClientLoginMaster *> loginServers;

    BaseClassSwitch::EpollObjectType getType() const;
    static int sendCurrentPlayer();//return the number if player
    static void sendTimeRangeEvent();
    static uint32_t maxAccountId;
    static bool havePlayerCountChange;

    struct DataForUpdatedServers
    {
        std::vector<EpollClientLoginMaster *> addServer;
        std::vector<uint8_t> removeServer;//flat index
    };
    static DataForUpdatedServers dataForUpdatedServers;
private:
    void parseNetworkReadError(const std::string &errorString);

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

    bool parseInputBeforeLogin(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size);
    void updateConsoleCountServer();

    friend class CheckTimeoutGameServer;
};
}

#endif // EPOLLCLIENTLOGINMASTER_H
