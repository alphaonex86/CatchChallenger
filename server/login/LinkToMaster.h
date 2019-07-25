#ifndef LOGINLINKTOMASTER_H
#define LOGINLINKTOMASTER_H

#include "../../general/base/ProtocolParsing.h"
#include "../epoll/EpollClient.h"
#include <vector>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

namespace CatchChallenger {
class LinkToMaster : public EpollClient, public ProtocolParsingInputOutput
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

    static LinkToMaster *linkToMaster;
    static int linkToMasterSocketFd;
    static bool haveTheFirstSslHeader;
    static unsigned char header_magic_number[11];
    static unsigned char private_token_master[TOKEN_SIZE_FOR_MASTERAUTH];
    static unsigned char private_token_statclient[TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    static unsigned char queryNumberToCharacterGroup[CATCHCHALLENGER_MAXPROTOCOLQUERY];

    std::vector<uint8_t> queryNumberList;
    BaseClassSwitch::EpollObjectType getType() const;
    void parseIncommingData();
    static int tryConnect(const char * const host,const uint16_t &port,const uint8_t &tryInterval=1,const uint8_t &considerDownAfterNumberOfTry=30);
    bool trySelectCharacter(void * const client,const uint8_t &client_query_id,const uint32_t &serverUniqueKey,const uint8_t &charactersGroupIndex,const uint32_t &characterId);
    void sendProtocolHeader();
    void tryReconnect();
    void setConnexionSettings();
    void connectInternal();
    void readTheFirstSslHeader();
    bool sendRawBlock(const char * const data,const unsigned int &size);
    std::string listTheRunningQuery() const;

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

    bool parseInputBeforeLogin(const uint8_t &mainCodeType, const uint8_t &, const char * const, const unsigned int &);
private:
    static char host[256];
    static uint16_t port;
};
}

#endif // LOGINLINKTOMASTER_H
