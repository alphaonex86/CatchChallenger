#ifndef EPOLLCLIENTLOGINMASTER_H
#define EPOLLCLIENTLOGINMASTER_H

#include "../../general/base/ProtocolParsing.h"
#include "CharactersGroup.h"
#include "../VariableServer.h"

#include <random>
#include <QString>
#include <QRegularExpression>

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
    void parseIncommingData();
    void selectCharacter(const uint8_t &query_id, const uint32_t &serverUniqueKey, const uint8_t &charactersGroupIndex, const uint32_t &characterId, const uint32_t &accountId);
    bool trySelectCharacterGameServer(EpollClientLoginMaster * const loginServer,const uint8_t &client_query_id,const uint32_t &serverUniqueKey,const uint8_t &charactersGroupIndex,const uint32_t &characterId, const uint32_t &accountId);
    void selectCharacter_ReturnToken(const uint8_t &query_id,const char * const token);
    void selectCharacter_ReturnFailed(const uint8_t &query_id, const uint8_t &errorCode);
    void disconnectForDuplicateConnexionDetected(const uint32_t &characterId);
    static void broadcastGameServerChange();
    bool sendRawSmallPacket(const char * const data,const int &size);
    enum EpollClientLoginMasterStat
    {
        None,
        Logged,
        GameServer,
        LoginServer,
    };

    static bool currentPlayerForGameServerToUpdate;
    EpollClientLoginMasterStat stat;
    std::mt19937 rng;
    char *socketString;
    int socketStringSize;
    CharactersGroup *charactersGroupForGameServer;
    CharactersGroup::InternalGameServer *charactersGroupForGameServerInformation;
    struct DataForSelectedCharacterReturn
    {
        EpollClientLoginMaster * loginServer;
        uint8_t client_query_id;
        uint32_t serverUniqueKey;
        uint8_t charactersGroupIndex;
        uint32_t characterId;
    };
    //to unordered reply
    //QHash<uint8_t,DataForSelectedCharacterReturn> loginServerReturnForCharaterSelect;
    //to ordered reply
    QList<DataForSelectedCharacterReturn> loginServerReturnForCharaterSelect;
    std::vector<uint8_t> queryNumberList;
    QByteArray tokenForAuth;

    static char private_token[TOKEN_SIZE_FOR_MASTERAUTH];
    static const unsigned char protocolHeaderToMatch[BASE_PROTOCOL_MAGIC_SIZE];
    static unsigned char protocolReplyProtocolNotSupported[4];
    static unsigned char protocolReplyWrongAuth[4];
    static unsigned char protocolReplyCompressionNone[4+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    static unsigned char protocolReplyCompresssionZlib[4+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    static unsigned char protocolReplyCompressionXz[4+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    static char characterSelectionIsWrongBufferCharacterNotFound[64];
    static char characterSelectionIsWrongBufferCharacterAlreadyConnectedOnline[64];
    static char characterSelectionIsWrongBufferServerInternalProblem[64];
    static char characterSelectionIsWrongBufferServerNotFound[64];
    static uint8_t characterSelectionIsWrongBufferSize;
    static char selectCharaterRequestOnGameServer[3/*header*/+CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER];
    static unsigned char duplicateConnexionDetected[2/*header*/+4];
    static unsigned char getTokenForCharacterSelect[3/*header*/+4+4];
    static unsigned char replyToRegisterLoginServer[sizeof(uint8_t)+sizeof(uint8_t)+sizeof(uint8_t)+sizeof(uint8_t)+sizeof(uint16_t)+sizeof(uint8_t)+sizeof(uint8_t)+sizeof(uint8_t)
    +sizeof(uint32_t)*CATCHCHALLENGER_SERVER_MAXIDBLOCK+sizeof(uint32_t)*CATCHCHALLENGER_SERVER_MAXIDBLOCK+sizeof(uint32_t)*CATCHCHALLENGER_SERVER_MAXIDBLOCK
    +16*1024];
    static char tempBuffer[16*4096];
    static char tempBuffer2[16*4096];
    static unsigned char replyToRegisterLoginServerBaseOffset;
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
    static unsigned char replyToIdListBuffer[sizeof(uint8_t)+sizeof(uint8_t)+1024];//reply for 07
    static QHash<QString,int> logicalGroupHash;

    static FILE *fpRandomFile;
    static QList<EpollClientLoginMaster *> gameServers;
    static QList<EpollClientLoginMaster *> loginServers;

    BaseClassSwitch::EpollObjectType getType() const;
    static void sendCurrentPlayer();
    static uint32_t maxAccountId;
private:
    void parseNetworkReadError(const QString &errorString);

    void errorParsingLayer(const QString &error);
    void messageParsingLayer(const QString &message) const;
    void errorParsingLayer(const char * const error);
    void messageParsingLayer(const char * const message) const;
    //have message without reply
    void parseMessage(const uint8_t &mainCodeType,const char *data,const unsigned int &size);
    void parseFullMessage(const uint8_t &mainCodeType,const uint8_t &subCodeType,const char *data,const unsigned int &size);
    //have query with reply
    void parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char *data,const unsigned int &size);
    void parseFullQuery(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char *data,const unsigned int &size);
    //send reply
    void parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char *data,const unsigned int &size);
    void parseFullReplyData(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char *data,const unsigned int &size);

    void parseInputBeforeLogin(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size);
    void disconnectClient();
};
}

#endif // EPOLLCLIENTLOGINMASTER_H
