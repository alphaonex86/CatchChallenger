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
        Reconnecting,
    };
    Stat stat;
    enum GameServerMode
    {
        None,
        Proxy,
        Reconnect,
    };
    GameServerMode gameServerMode;
    struct ServerReconnect
    {
        std::string host;
        uint16_t port;
    };
    std::unordered_map<uint8_t/*charactersGroupIndex*/,std::unordered_map<uint32_t/*unique key*/,ServerReconnect> > serverReconnectList;
    ServerReconnect selectedServer;

    EpollClientLoginSlave *client;
    bool haveTheFirstSslHeader;
    uint8_t protocolQueryNumber;
    static QByteArray httpDatapackMirrorRewriteBase;
    static QByteArray httpDatapackMirrorRewriteMainAndSub;
    static bool compressionSet;
    static std::string mDatapackBase;
    std::string main;
    std::string sub;

    void setConnexionSettings();
    BaseClassSwitch::EpollObjectType getType() const;
    void parseIncommingData();
    static int tryConnect(const char * const host,const uint16_t &port,const uint8_t &tryInterval=1,const uint8_t &considerDownAfterNumberOfTry=30);
    void sendProtocolHeader();
    void sendProtocolHeaderGameServer();
    void sendDiffered04Reply();
    void sendDiffered0205Reply();
    void readTheFirstSslHeader();
    void disconnectClient();
    uint8_t freeQueryNumberToServer();
    bool sendRawSmallPacket(const char * const data,const int &size);
protected:
    void errorParsingLayer(const std::string &error);
    void messageParsingLayer(const std::string &message) const;
    void errorParsingLayer(const char * const error);
    void messageParsingLayer(const char * const message) const;
    void parseNetworkReadError(const std::string &errorString);

    //have message without reply
    bool parseMessage(const uint8_t &mainCodeType,const char * const data,const unsigned int &size);
    //have query with reply
    bool parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size);
    //send reply
    bool parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size);

    bool parseInputBeforeLogin(const uint8_t &mainCodeType, const uint8_t &queryNumber, const char * const data, const unsigned int &size);
private:
    int socketFd;
    char *reply04inWait;
    unsigned int reply04inWaitSize;
    uint8_t reply04inWaitQueryNumber;
    char *reply0205inWait;
    unsigned int reply0205inWaitSize;
    uint8_t reply0205inWaitQueryNumber;
    uint8_t queryIdToReconnect;
    char tokenForGameServer[CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER];
};
}

#endif // LOGINLINKTOGameServer_H
