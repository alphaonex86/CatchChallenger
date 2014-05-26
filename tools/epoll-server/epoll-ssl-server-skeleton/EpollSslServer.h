#ifndef EPOLL_SSL_SERVER_H
#define EPOLL_SSL_SERVER_H

#ifndef SERVERNOSSL
#include <sys/socket.h>
#include <openssl/ssl.h>

#include "BaseClassSwitch.h"

class EpollSslServer : public BaseClassSwitch
{
public:
    EpollSslServer();
    ~EpollSslServer();
    bool tryListen(char *port);
    void close();
    int accept(sockaddr *in_addr,socklen_t *in_len);
    int getSfd();
    Type getType() const;
    SSL_CTX * getCtx() const;
private:
    int sfd;
    SSL_CTX *ctx;
private:
    void initSslPart();
    SSL_CTX* InitServerCTX();
    void LoadCertificates(SSL_CTX* ctx, const char* CertFile, const char* KeyFile);
};
#endif

#endif // EPOLL_SSL_SERVER_H
