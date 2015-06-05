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
    void selectCharacter(const quint8 &query_id,const quint32 &serverUniqueKey,const quint8 &charactersGroupIndex,const quint32 &characterId);
    bool trySelectCharacterGameServer(EpollClientLoginMaster * const loginServer,const quint8 &client_query_id,const quint32 &serverUniqueKey,const quint8 &charactersGroupIndex,const quint32 &characterId);
    void selectCharacter_ReturnToken(const quint8 &query_id,const char * const token);
    void selectCharacter_ReturnFailed(const quint8 &query_id, const quint8 &errorCode, const quint32 &characterId);
    static void broadcastGameServerChange();
    bool sendRawSmallPacket(const char * const data,const int &size);
    enum EpollClientLoginMasterStat
    {
        None,
        Logged,
        GameServer,
        LoginServer,
    };

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

    static char private_token[TOKEN_SIZE_FOR_MASTERAUTH];
    static const unsigned char protocolHeaderToMatch[BASE_PROTOCOL_MAGIC_SIZE];
    static unsigned char protocolReplyProtocolNotSupported[3];
    static unsigned char protocolReplyWrongAuth[3];
    static unsigned char protocolReplyCompressionNone[3];
    static unsigned char protocolReplyCompresssionZlib[3];
    static unsigned char protocolReplyCompressionXz[3];
    static unsigned char loginIsWrongBuffer[4];
    static char selectCharaterRequest[3+4];
    static unsigned char replyToRegisterLoginServer[sizeof(quint8)+sizeof(quint8)+sizeof(quint8)+sizeof(quint16)+sizeof(quint8)+sizeof(quint8)+sizeof(quint8)
    +sizeof(quint32)*CATCHCHALLENGER_SERVER_MAXIDBLOCK+sizeof(quint32)*CATCHCHALLENGER_SERVER_MAXIDBLOCK+sizeof(quint32)*CATCHCHALLENGER_SERVER_MAXIDBLOCK
    +1000];
    static char tempBuffer[4096];
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
    static unsigned char replyToIdListBuffer[sizeof(quint8)+sizeof(quint8)+sizeof(quint32)*CATCHCHALLENGER_SERVER_MAXIDBLOCK];

    static QList<EpollClientLoginMaster *> gameServers;
    static QList<EpollClientLoginMaster *> loginServers;

    BaseClassSwitch::Type getType() const;
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
