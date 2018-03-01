#ifndef LOGINLINKTOMASTER_H
#define LOGINLINKTOMASTER_H

#include "../../general/base/ProtocolParsing.h"
#include "TimerReconnectOnTheMaster.h"
#include "../base/TinyXMLSettings.h"
#include <vector>
#include <random>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

namespace CatchChallenger {
class LinkToMaster : public BaseClassSwitch, public ProtocolParsingInputOutput
{
public:
    explicit LinkToMaster(
        #ifdef SERVERSSL
            const int &infd, SSL_CTX *ctx
        #else
            const int &infd
        #endif
            );
    ~LinkToMaster();
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
    struct DataForSelectedCharacterReturn
    {
        void * client;
        uint8_t client_query_id;
        uint32_t serverUniqueKey;
        uint8_t charactersGroupIndex;
    };

    void *baseServer;
    std::string httpDatapackMirror;
    //to unordered reply
    std::unordered_map<uint8_t/*queryNumber*/,DataForSelectedCharacterReturn> selectCharacterClients;
    static char protocolReplyNoMoreToken[7];
    static char protocolReplyAlreadyConnectedToken[7];
    static char protocolReplyGetToken[6+CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER];
    static char protocolReplyPing[2];
    static char sendDisconnectedPlayer[1+4];
    static char sendCurrentPlayer[1+2];
    static unsigned char header_magic_number[9];
    static unsigned char private_token[TOKEN_SIZE_FOR_MASTERAUTH];

    static uint16_t purgeLockPeriod;
    static uint16_t maxLockAge;
    static LinkToMaster *linkToMaster;
    static int linkToMasterSocketFd;
    static bool haveTheFirstSslHeader;
    std::vector<uint8_t> queryNumberList;
    BaseClassSwitch::EpollObjectType getType() const;
    void parseIncommingData();
    static int tryConnect(const char * const host,const uint16_t &port,const uint8_t &tryInterval=1,const uint8_t &considerDownAfterNumberOfTry=30);
    void connectInternal();
    void setConnexionSettings(const uint8_t &tryInterval, const uint8_t &considerDownAfterNumberOfTry);
    bool registerGameServer(const std::string &exportedXml,const char * const dynamicToken);
    void generateToken(TinyXMLSettings * const settings);
    void sendProtocolHeader();
    bool setSettings(TinyXMLSettings * const settings);
    void characterDisconnected(const uint32_t &characterId);
    void currentPlayerChange(const uint16_t &currentPlayer);
    bool askMoreMaxMonsterId();
    bool askMoreMaxClanId();
    void tryReconnect();
    void readTheFirstSslHeader();
    void moveClientFastPath(const uint8_t &, const uint8_t &);
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
    static sockaddr_in serv_addr;
    uint8_t tryInterval;
    uint8_t considerDownAfterNumberOfTry;
    std::string server_ip;
    std::string server_port;
    std::string externalServerIp;
    uint16_t externalServerPort;
    std::string charactersGroup;
    uint32_t uniqueKey;
    std::string logicalGroup;
    bool askMoreMaxClanIdInProgress;
    bool askMoreMaxMonsterIdInProgress;
};
}

#endif // LOGINLINKTOMASTER_H
