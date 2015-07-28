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
    void selectCharacter(const quint8 &query_id, const quint32 &serverUniqueKey, const quint8 &charactersGroupIndex, const quint32 &characterId, const quint32 &accountId);
    bool trySelectCharacterGameServer(EpollClientLoginMaster * const loginServer,const quint8 &client_query_id,const quint32 &serverUniqueKey,const quint8 &charactersGroupIndex,const quint32 &characterId, const quint32 &accountId);
    void selectCharacter_ReturnToken(const quint8 &query_id,const char * const token);
    void selectCharacter_ReturnFailed(const quint8 &query_id, const quint8 &errorCode);
    void disconnectForDuplicateConnexionDetected(const quint32 &characterId);
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
        quint8 client_query_id;
        quint32 serverUniqueKey;
        quint8 charactersGroupIndex;
        quint32 characterId;
    };
    //to unordered reply
    //QHash<quint8,DataForSelectedCharacterReturn> loginServerReturnForCharaterSelect;
    //to ordered reply
    QList<DataForSelectedCharacterReturn> loginServerReturnForCharaterSelect;
    std::vector<quint8> queryNumberList;
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
    static quint8 characterSelectionIsWrongBufferSize;
    static char selectCharaterRequestOnGameServer[3/*header*/+CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER];
    static unsigned char duplicateConnexionDetected[2/*header*/+4];
    static unsigned char getTokenForCharacterSelect[3/*header*/+4+4];
    static unsigned char replyToRegisterLoginServer[sizeof(quint8)+sizeof(quint8)+sizeof(quint8)+sizeof(quint8)+sizeof(quint16)+sizeof(quint8)+sizeof(quint8)+sizeof(quint8)
    +sizeof(quint32)*CATCHCHALLENGER_SERVER_MAXIDBLOCK+sizeof(quint32)*CATCHCHALLENGER_SERVER_MAXIDBLOCK+sizeof(quint32)*CATCHCHALLENGER_SERVER_MAXIDBLOCK
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
    static unsigned char replyToIdListBuffer[sizeof(quint8)+sizeof(quint8)+1024];//reply for 07
    static QHash<QString,int> logicalGroupHash;

    static FILE *fpRandomFile;
    static QList<EpollClientLoginMaster *> gameServers;
    static QList<EpollClientLoginMaster *> loginServers;

    BaseClassSwitch::EpollObjectType getType() const;
    static void sendCurrentPlayer();
    static quint32 maxAccountId;
private:
    void parseNetworkReadError(const QString &errorString);

    void errorParsingLayer(const QString &error);
    void messageParsingLayer(const QString &message) const;
    void errorParsingLayer(const char * const error);
    void messageParsingLayer(const char * const message) const;
    //have message without reply
    void parseMessage(const quint8 &mainCodeType,const char *data,const unsigned int &size);
    void parseFullMessage(const quint8 &mainCodeType,const quint8 &subCodeType,const char *data,const unsigned int &size);
    //have query with reply
    void parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const unsigned int &size);
    void parseFullQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char *data,const unsigned int &size);
    //send reply
    void parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const unsigned int &size);
    void parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char *data,const unsigned int &size);

    void parseInputBeforeLogin(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const unsigned int &size);
    void disconnectClient();
};
}

#endif // EPOLLCLIENTLOGINMASTER_H
