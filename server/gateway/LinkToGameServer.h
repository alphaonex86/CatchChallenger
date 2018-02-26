#ifndef LOGINLINKTOGameServer_H
#define LOGINLINKTOGameServer_H

#include "../../general/base/ProtocolParsing.h"
#include <vector>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <regex>

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
        Unconnected=0,
        Connecting=1,
        Connected=2,
        ProtocolGood=3,
        Reconnecting=4,
    };
    Stat stat;
    enum GameServerMode
    {
        None=0,
        Proxy=1,
        Reconnect=2,
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
    static std::vector<char> httpDatapackMirrorRewriteBase;
    static std::vector<char> httpDatapackMirrorRewriteMainAndSub;
    static bool compressionSet;
    static std::string mDatapackBase;
    std::string main;
    std::string sub;
    static std::string lastconnectErrorMessage;
    static const std::regex mainDatapackCodeFilter;
    static const std::regex subDatapackCodeFilter;
    static const std::string protocolString;
    static const std::string protocolHttp;
    static const std::string protocolHttps;

    void setConnexionSettings();
    BaseClassSwitch::EpollObjectType getType() const;
    void parseIncommingData();
    static int tryConnect(const char * const host,const uint16_t &port,const uint8_t &tryInterval=1,const uint8_t &considerDownAfterNumberOfTry=30);
    void sendProtocolHeader();
    void sendProtocolHeaderGameServer();
    void sendDifferedA8Reply();
    void sendDiffered93OrACReply();
    void readTheFirstSslHeader();
    bool disconnectClient();
    uint8_t freeQueryNumberToServer();
    bool sendRawSmallPacket(const char * const data,const int &size);
    bool removeFromQueryReceived(const uint8_t &queryNumber);
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
    char *replySelectListInWait;
    unsigned int replySelectListInWaitSize;
    uint8_t replySelectListInWaitQueryNumber;
    char *replySelectCharInWait;
    unsigned int replySelectCharInWaitSize;
    uint8_t replySelectCharInWaitQueryNumber;
    uint8_t queryIdToReconnect;
    char tokenForGameServer[CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER];
};
}

#endif // LOGINLINKTOGameServer_H
