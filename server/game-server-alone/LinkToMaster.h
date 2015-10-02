#ifndef LOGINLINKTOMASTER_H
#define LOGINLINKTOMASTER_H

#include "../../general/base/ProtocolParsing.h"
#include "TimerReconnectOnTheMaster.h"
#include <vector>
#include <random>
#include <std::unordered_settings>
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

    std::string httpDatapackMirror;
    //to unordered reply
    std::unordered_map<uint8_t/*queryNumber*/,DataForSelectedCharacterReturn> selectCharacterClients;
    static char protocolReplyNoMoreToken[4];
    static char protocolReplyAlreadyConnectedToken[4];
    static char protocolReplyGetToken[3+CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER];
    static char sendDisconnectedPlayer[2+4];
    static char sendCurrentPlayer[2+2];
    static unsigned char header_magic_number[9];
    static unsigned char private_token[TOKEN_SIZE_FOR_MASTERAUTH];

    static LinkToMaster *linkToMaster;
    static int linkToMasterSocketFd;
    static bool haveTheFirstSslHeader;
    std::vector<uint8_t> queryNumberList;
    BaseClassSwitch::EpollObjectType getType() const;
    void parseIncommingData();
    static int tryConnect(const char * const host,const uint16_t &port,const uint8_t &tryInterval=1,const uint8_t &considerDownAfterNumberOfTry=30);
    void connectInternal();
    void setConnexionSettings();
    bool registerGameServer(const std::string &exportedXml,const char * const dynamicToken);
    void generateToken();
    void sendProtocolHeader();
    bool setSettings(std::unordered_settings * const settings);
    void characterDisconnected(const uint32_t &characterId);
    void currentPlayerChange(const uint16_t &currentPlayer);
    void askMoreMaxMonsterId();
    void askMoreMaxClanId();
    void tryReconnect();
    void readTheFirstSslHeader();
protected:
    void disconnectClient();
    void errorParsingLayer(const std::string &error);
    void messageParsingLayer(const std::string &message) const;
    void errorParsingLayer(const char * const error);
    void messageParsingLayer(const char * const message) const;
    void parseNetworkReadError(const std::string &errorString);

    //have message without reply
    void parseMessage(const uint8_t &mainCodeType,const char * const data,const unsigned int &size);
    void parseFullMessage(const uint8_t &mainCodeType,const uint8_t &subCodeType,const char * const data,const unsigned int &size);
    //have query with reply
    void parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size);
    void parseFullQuery(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size);
    //send reply
    void parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size);
    void parseFullReplyData(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size);

    void parseInputBeforeLogin(const uint8_t &mainCodeType, const uint8_t &queryNumber, const char * const data, const unsigned int &size);
private:
    std::unordered_settings * settings;
    std::mt19937 rng;
    static sockaddr_in serv_addr;
};
}

#endif // LOGINLINKTOMASTER_H
