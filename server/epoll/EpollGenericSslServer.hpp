#ifdef SERVERSSL

#ifndef EPOLLGENERICSSLSERVER_H
#define EPOLLGENERICSSLSERVER_H

#include "EpollGenericServer.h"

#include <openssl/ssl.h>

namespace CatchChallenger {
class EpollGenericSslServer : public CatchChallenger::EpollGenericServer
{
public:
    EpollGenericSslServer();
    bool trySslListen(const char* const ip,const char* const port, const char* const CertFile, const char* const KeyFile);
    SSL_CTX * getCtx() const;
private:
    SSL_CTX *ctx;
private:
    void initSslPart(const char* const CertFile, const char* const KeyFile);
    virtual SSL_CTX* InitServerCTX();
    void LoadCertificates(SSL_CTX* ctx, const char* const CertFile, const char* const KeyFile);
};
}


#endif // EPOLLGENERICSSLSERVER_H
#endif // SERVERSSL
