#ifndef EPOLLCLIENTLOGINMASTER_H
#define EPOLLCLIENTLOGINMASTER_H

#include "../epoll/EpollClient.h"
#include "../epoll/EpollSslClient.h"
#include "../../general/base/ProtocolParsing.h"

#include <QString>

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
    enum EpollClientLoginMasterStat
    {
        None,
        Logged,
        GameServer,
        LoginServer,
    };

    EpollClientLoginMasterStat stat;
    char *socketString;
    int socketStringSize;

    static char private_token[TOKEN_SIZE];
    static const unsigned char protocolHeaderToMatch[BASE_PROTOCOL_MAGIC_SIZE];
    static unsigned char protocolReplyProtocolNotSupported[3];
    static unsigned char protocolReplyWrongAuth[3];
    static unsigned char protocolReplyCompressionNone[3];
    static unsigned char protocolReplyCompresssionZlib[3];
    static unsigned char protocolReplyCompressionXz[3];
    static unsigned char replyToRegisterLoginServer[sizeof(quint8)+sizeof(quint8)+sizeof(quint8)+sizeof(quint16)+sizeof(quint8)+sizeof(quint8)+sizeof(quint8)
    +sizeof(quint32)*CATCHCHALLENGER_SERVER_MAXIDBLOCK+sizeof(quint32)*CATCHCHALLENGER_SERVER_MAXIDBLOCK+sizeof(quint32)*CATCHCHALLENGER_SERVER_MAXIDBLOCK];
    static unsigned char replyToIdListBuffer[sizeof(quint8)+sizeof(quint8)+sizeof(quint32)*CATCHCHALLENGER_SERVER_MAXIDBLOCK];

    BaseClassSwitch::Type getType() const;
    static quint32 maxClanId;
    static quint32 maxAccountId;
    static quint32 maxCharacterId;
    static quint32 maxMonsterId;
private:
    void parseNetworkReadError(const QString &errorString);

    void errorParsingLayer(const QString &error);
    void messageParsingLayer(const QString &message) const;
    void errorParsingLayer(const char * const error);
    void messageParsingLayer(const char * const message) const;
    //have message without reply
    void parseMessage(const quint8 &mainCodeType,const char *data,const int &size);
    void parseFullMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const char *data,const int &size);
    //have query with reply
    void parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size);
    void parseFullQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size);
    //send reply
    void parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size);
    void parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size);

    void parseInputBeforeLogin(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size);
    void disconnectClient();
};
}

#endif // EPOLLCLIENTLOGINMASTER_H
