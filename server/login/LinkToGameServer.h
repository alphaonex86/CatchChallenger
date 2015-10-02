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
        LoginInProgress,
        Logged,
    };
    Stat stat;

    char tokenForGameServer[CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER];
    EpollClientLoginSlave *client;
    bool haveTheFirstSslHeader;
    static const unsigned char protocolHeaderToMatchGameServer[5];
    uint8_t queryIdToReconnect;

    void setConnexionSettings();
    BaseClassSwitch::EpollObjectType getType() const;
    void parseIncommingData();
    static int tryConnect(const char * const host,const uint16_t &port,const uint8_t &tryInterval=1,const uint8_t &considerDownAfterNumberOfTry=30);
    bool trySelectCharacter(void * const client,const uint8_t &client_query_id,const uint32_t &serverUniqueKey,const uint8_t &charactersGroupIndex,const uint32_t &characterId);
    void sendProtocolHeader();
    void readTheFirstSslHeader();
protected:
    void disconnectClient();
    void errorParsingLayer(const std::string &error);
    void messageParsingLayer(const std::string &message) const;
    void errorParsingLayer(const char * const error);
    void messageParsingLayer(const char * const message) const;
    void parseNetworkReadError(const std::string &errorString);

    //have message without reply
    void parseMessage(const uint8_t &mainCodeType,const char *data,const unsigned int &size);
    void parseFullMessage(const uint8_t &mainCodeType,const uint8_t &subCodeType,const char *data,const unsigned int &size);
    //have query with reply
    void parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char *data,const unsigned int &size);
    void parseFullQuery(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char *data,const unsigned int &size);
    //send reply
    void parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char *data,const unsigned int &size);
    void parseFullReplyData(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char *data,const unsigned int &size);

    void parseInputBeforeLogin(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char *data,const unsigned int &size);
private:
    int socketFd;
};
}

#endif // LOGINLINKTOGameServer_H
