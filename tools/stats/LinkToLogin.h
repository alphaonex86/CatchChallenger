#ifndef LOGINLINKTOLOGIN_H
#define LOGINLINKTOLOGIN_H

#include "../../general/base/ProtocolParsing.h"
#include "../../server/base/TinyXMLSettings.h"
#include "../../server/epoll/EpollClient.h"
#include <vector>
#include <random>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unordered_map>
#include <vector>

namespace CatchChallenger {
class LinkToLogin : public EpollClient, public ProtocolParsingInputOutput
{
public:
    explicit LinkToLogin(
        #ifdef SERVERSSL
            const int &infd, SSL_CTX *ctx
        #else
            const int &infd
        #endif
            );
    ~LinkToLogin();
    enum Stat
    {
        Unconnected=0,
        Connecting=1,
        Connected=2,
        ProtocolSent=3,
        ProtocolGood=4,
        LoginInProgress=5,
        Logged=6,
        //can't be greater to 9 due replyNotConnected[41]=X for the unix socket
    };
    Stat stat;
    struct DataForSelectedCharacterReturn
    {
        void * client;
        uint8_t client_query_id;
        uint32_t serverUniqueKey;
        uint8_t charactersGroupIndex;
    };
    enum ProxyMode
    {
        Reconnect=0x01,
        Proxy=0x02
    };
    ProxyMode proxyMode;
    struct ServerFromPoolForDisplay
    {
        //connect info
        uint32_t uniqueKey;

        //displayed info
        std::string xml;
        uint16_t maxPlayer;
        uint16_t currentPlayer;

        //other
        uint8_t groupIndex;
        uint8_t logicalGroupIndex;
    };
    struct LogicalGroup
    {
        std::string path;
        std::string xml;
    };

    //to unordered reply
    std::unordered_map<uint8_t/*queryNumber*/,DataForSelectedCharacterReturn> selectCharacterClients;
    static unsigned char header_magic_number[5];
    static unsigned char private_token_statclient[TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];

    #ifndef STATSODROIDSHOW2
    static FILE * pFile;
    #else
    static int usbdev;
    #endif
    static std::string pFilePath;
    #ifndef STATSODROIDSHOW2
    static LinkToLogin *linkToLogin;
    #endif
    static bool withIndentation;
    static int linkToLoginSocketFd;
    static bool haveTheFirstSslHeader;
    std::vector<uint8_t> queryNumberList;
    BaseClassSwitch::EpollObjectType getType() const;
    void parseIncommingData();
    static int tryConnect(const char * const host,const uint16_t &port,const uint8_t &tryInterval=1,const uint8_t &considerDownAfterNumberOfTry=30);
    void connectInternal();
    void setConnexionSettings(const uint8_t &tryInterval, const uint8_t &considerDownAfterNumberOfTry);
    bool registerStatsClient(const char * const dynamicToken);
    void sendProtocolHeader();
    virtual void tryReconnect();
    void readTheFirstSslHeader();
    void moveClientFastPath(const uint8_t &, const uint8_t &);
    virtual void updateJsonFile(const bool &withIndentation);
    #ifndef STATSODROIDSHOW2
    static void removeJsonFile();
    #else
    static void writeData(const char * const str);
    static void writeData(const std::string &str);
    #endif
    static void displayErrorAndQuit(const char * errorString);
    const std::string &getJsonFileContent() const;

    ssize_t read(char * data, const size_t &size);
    ssize_t write(const char * const data, const size_t &size);
    void closeSocket();
protected:
    bool disconnectClient();
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
    std::mt19937 rng;
    uint8_t tryInterval;
    uint8_t considerDownAfterNumberOfTry;
    static char host[255];
    static uint16_t port;
    std::string jsonFileContent;
    std::vector<LogicalGroup> logicalGroups;
    bool warningLogicalGroupOutOfBound;
protected:
    std::vector<ServerFromPoolForDisplay> serverList;
};
}

#endif // LOGINLINKTOMASTER_H
