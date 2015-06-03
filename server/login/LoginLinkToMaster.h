#ifndef LOGINLINKTOMASTER_H
#define LOGINLINKTOMASTER_H

#include "../../general/base/ProtocolParsing.h"
#include <vector>

namespace CatchChallenger {
class LoginLinkToMaster : public BaseClassSwitch, public ProtocolParsingInputOutput
{
public:
    explicit LoginLinkToMaster(
        #ifdef SERVERSSL
            const int &infd, SSL_CTX *ctx
        #else
            const int &infd
        #endif
            );
    ~LoginLinkToMaster();
    enum Stat
    {
        None,
        ProtocolGood,
        LoginInProgress,
        Logged,
    };
    Stat stat;
    struct DataForSelectedCharacterReturn
    {
        void * client;
        quint8 client_query_id;
        quint32 serverUniqueKey;
        quint8 charactersGroupIndex;
    };

    QString httpDatapackMirror;
    //to unordered reply
    QHash<quint8/*queryNumber*/,DataForSelectedCharacterReturn> selectCharacterClients;

    std::vector<quint8> queryNumberList;
    BaseClassSwitch::Type getType() const;
    void parseIncommingData();
    static int tryConnect(const char * const host,const quint16 &port,const quint8 &tryInterval=1,const quint8 &considerDownAfterNumberOfTry=30);
    bool trySelectCharacter(void * const client,const quint8 &client_query_id,const quint32 &serverUniqueKey,const quint8 &charactersGroupIndex,const quint32 &characterId);
    void sendProtocolHeader();
protected:
    void disconnectClient();
    void errorParsingLayer(const QString &error);
    void messageParsingLayer(const QString &message) const;
    void errorParsingLayer(const char * const error);
    void messageParsingLayer(const char * const message) const;
    void parseNetworkReadError(const QString &errorString);

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
};
}

#endif // LOGINLINKTOMASTER_H
