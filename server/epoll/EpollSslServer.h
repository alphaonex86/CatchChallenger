#ifndef EPOLL_SSL_SERVER_H
#define EPOLL_SSL_SERVER_H

#ifndef SERVERNOSSL
#include <sys/socket.h>
#include <openssl/ssl.h>

#include "BaseClassSwitch.h"
#include "../base/BaseServer.h"
#include "../base/ServerStructures.h"

namespace CatchChallenger {
class EpollSslServer : public BaseClassSwitch, public CatchChallenger::BaseServer
{
public:
    EpollSslServer();
    ~EpollSslServer();
    bool tryListen();
    void close();
    int accept(sockaddr *in_addr,socklen_t *in_len);
    int getSfd();
    Type getType() const;
    SSL_CTX * getCtx() const;
    void preload_the_data();
    CatchChallenger::ClientMapManagement * getClientMapManagement();
    void unload_the_data();
    void setNormalSettings(const NormalServerSettings &settings);
    NormalServerSettings getNormalSettings() const;
    void loadAndFixSettings();
private:
    int sfd;
    SSL_CTX *ctx;
    NormalServerSettings normalServerSettings;
    int yes;
private:
    void initSslPart();
    SSL_CTX* InitServerCTX();
    void LoadCertificates(SSL_CTX* ctx, const char* CertFile, const char* KeyFile);
};
}
#endif

#endif // EPOLL_SSL_SERVER_H
