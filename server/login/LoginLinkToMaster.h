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
    std::vector<quint8> queryNumberList;
    BaseClassSwitch::Type getType() const;
    void parseIncommingData();
    static int tryConnect(const char * const host,const quint16 &port,const quint8 &tryInterval=1,const quint8 &considerDownAfterNumberOfTry=30);
protected:
    void disconnectClient();
    void errorParsingLayer(const QString &error);
    void messageParsingLayer(const QString &message) const;
    void errorParsingLayer(const char * const error);
    void messageParsingLayer(const char * const message) const;

    //have message without reply
    void parseMessage(const quint8 &mainCodeType,const char *data,const int &size);
    void parseFullMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const char *data,const int &size);
    //have query with reply
    void parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size);
    void parseFullQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size);
    //send reply
    void parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size);
    void parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size);
};
}

#endif // LOGINLINKTOMASTER_H
