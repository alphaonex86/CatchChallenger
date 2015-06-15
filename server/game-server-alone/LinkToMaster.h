#ifndef LOGINLINKTOMASTER_H
#define LOGINLINKTOMASTER_H

#include "../../general/base/ProtocolParsing.h"
#include "TimerReconnectOnTheMaster.h"
#include <vector>
#include <random>
#include <QSettings>
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
        quint8 client_query_id;
        quint32 serverUniqueKey;
        quint8 charactersGroupIndex;
    };

    QString httpDatapackMirror;
    //to unordered reply
    QHash<quint8/*queryNumber*/,DataForSelectedCharacterReturn> selectCharacterClients;
    static char protocolReplyNoMoreToken[4];
    static char protocolReplyAlreadyConnectedToken[4];
    static char protocolReplyGetToken[3+CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER];
    static char sendDisconnectedPlayer[2+4];
    static char sendCurrentPlayer[2+2];
    static unsigned char header_magic_number_and_private_token[9+TOKEN_SIZE_FOR_MASTERAUTH];

    static LinkToMaster *linkToMaster;
    static int linkToMasterSocketFd;
    static bool haveTheFirstSslHeader;
    std::vector<quint8> queryNumberList;
    BaseClassSwitch::Type getType() const;
    void parseIncommingData();
    static int tryConnect(const char * const host,const quint16 &port,const quint8 &tryInterval=1,const quint8 &considerDownAfterNumberOfTry=30);
    void connectInternal();
    void setConnexionSettings();
    bool registerGameServer(const QString &exportedXml);
    void generateToken();
    void sendProtocolHeader();
    bool setSettings(QSettings * const settings);
    void characterDisconnected(const quint32 &characterId);
    void currentPlayerChange(const quint16 &currentPlayer);
    void askMoreMaxMonsterId();
    void askMoreMaxClanId();
    void tryReconnect();
    void readTheFirstSslHeader();
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
private:
    QSettings * settings;
    std::mt19937 rng;
    static sockaddr_in serv_addr;
};
}

#endif // LOGINLINKTOMASTER_H
