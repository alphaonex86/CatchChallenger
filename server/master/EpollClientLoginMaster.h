#ifndef EPOLLCLIENTLOGINMASTER_H
#define EPOLLCLIENTLOGINMASTER_H

#include "../../general/base/ProtocolParsing.h"
#include "CharactersGroup.h"

#include <QString>
#include <QRegularExpression>

#define BASE_PROTOCOL_MAGIC_SIZE 8
#define TOKEN_SIZE 64
#define CATCHCHALLENGER_SERVER_MAXIDBLOCK 50

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
    void parseIncommingData();
    void selectCharacter(const quint8 &query_id,const quint32 &serverUniqueKey,const quint8 &charactersGroupIndex,const quint32 &characterId);
    bool trySelectCharacter(EpollClientLoginMaster * const loginServer,const quint8 &client_query_id,const quint32 &serverUniqueKey,const quint8 &charactersGroupIndex,const quint32 &characterId);
    void selectCharacter_ReturnToken(const quint8 &query_id,const char * const token);
    void selectCharacter_ReturnFailed(const quint8 &query_id, const quint8 &errorCode, const quint32 &characterId);
    enum EpollClientLoginMasterStat
    {
        None,
        Logged,
        GameServer,
        LoginServer,
    };
    ~EpollClientLoginMaster();

    EpollClientLoginMasterStat stat;
    char *socketString;
    int socketStringSize;
    CharactersGroup *charactersGroupForGameServer;
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

    static bool automatic_account_creation;
    static char private_token[TOKEN_SIZE];
    static const unsigned char protocolHeaderToMatch[BASE_PROTOCOL_MAGIC_SIZE];
    static unsigned char protocolReplyProtocolNotSupported[3];
    static unsigned char protocolReplyWrongAuth[3];
    static unsigned char protocolReplyCompressionNone[3];
    static unsigned char protocolReplyCompresssionZlib[3];
    static unsigned char protocolReplyCompressionXz[3];
    static unsigned char loginIsWrongBuffer[4];
    static char selectCharaterRequest[3+4+1+4];
    static unsigned char replyToRegisterLoginServer[sizeof(quint8)+sizeof(quint8)+sizeof(quint8)+sizeof(quint16)+sizeof(quint8)+sizeof(quint8)+sizeof(quint8)
    +sizeof(quint32)*CATCHCHALLENGER_SERVER_MAXIDBLOCK+sizeof(quint32)*CATCHCHALLENGER_SERVER_MAXIDBLOCK+sizeof(quint32)*CATCHCHALLENGER_SERVER_MAXIDBLOCK
    +1000];
    static unsigned char replyToRegisterLoginServerBaseOffset;
    static char loginSettingsAndCharactersGroup[256*1024];
    static int loginSettingsAndCharactersGroupSize;
    static char serverPartialServerList[256*1024];
    static int serverPartialServerListSize;
    static char serverServerList[256*1024];
    static int serverServerListSize;
    static char serverLogicalGroupList[256*1024];
    static int serverLogicalGroupListSize;
    static char loginPreviousToReplyCache[256*1024*3];
    static int loginPreviousToReplyCacheSize;
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
