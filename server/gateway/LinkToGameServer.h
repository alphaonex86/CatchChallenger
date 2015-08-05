#ifndef LOGINLINKTOGameServer_H
#define LOGINLINKTOGameServer_H

#include "../../general/base/ProtocolParsing.h"
#include <vector>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

namespace CatchChallenger {
class EpollClientLoginSlave;
class LinkToGameServer : public BaseClassSwitch, public ProtocolParsingInputOutput
{
public:
    explicit LinkToGameServer(
        #ifdef SERVERSSL
            const int &infd, SSL_CTX *ctx
        #else
            const int &infd
        #endif
            );
    ~LinkToGameServer();
    enum Stat
    {
        Unconnected,
        Connecting,
        Connected,
        ProtocolGood,
    };
    Stat stat;

    EpollClientLoginSlave *client;
    bool haveTheFirstSslHeader;
    quint8 protocolQueryNumber;
    static QByteArray httpDatapackMirrorRewriteBase;
    static QByteArray httpDatapackMirrorRewriteMainAndSub;
    static bool compressionSet;
    static QString mDatapackBase;

    void setConnexionSettings();
    BaseClassSwitch::EpollObjectType getType() const;
    void parseIncommingData();
    static int tryConnect(const char * const host,const quint16 &port,const quint8 &tryInterval=1,const quint8 &considerDownAfterNumberOfTry=30);
    bool trySelectCharacter(void * const client,const quint8 &client_query_id,const quint32 &serverUniqueKey,const quint8 &charactersGroupIndex,const quint32 &characterId);
    void sendProtocolHeader();
    void sendDiffered04Reply();
    void sendDiffered0205Reply();
    void readTheFirstSslHeader();
    void disconnectClient();
    quint8 freeQueryNumberToServer();
protected:
    void errorParsingLayer(const QString &error);
    void messageParsingLayer(const QString &message) const;
    void errorParsingLayer(const char * const error);
    void messageParsingLayer(const char * const message) const;
    void parseNetworkReadError(const QString &errorString);

    //have message without reply
    void parseMessage(const quint8 &mainCodeType,const char * const data,const unsigned int &size);
    void parseFullMessage(const quint8 &mainCodeType,const quint8 &subCodeType,const char * const data,const unsigned int &size);
    //have query with reply
    void parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size);
    void parseFullQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size);
    //send reply
    void parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size);
    void parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size);

    void parseInputBeforeLogin(const quint8 &mainCodeType, const quint8 &queryNumber, const char * const data, const unsigned int &size);
private:
    int socketFd;
    char *reply04inWait;
    unsigned int reply04inWaitSize;
    quint8 reply04inWaitQueryNumber;
    char *reply0205inWait;
    unsigned int reply0205inWaitSize;
    quint8 reply0205inWaitQueryNumber;
    QString main;
    QString sub;
};
}

#endif // LOGINLINKTOGameServer_H
