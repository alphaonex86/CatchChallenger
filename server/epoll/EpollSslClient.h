#ifndef EPOLL_SSL_CLIENT_H
#define EPOLL_SSL_CLIENT_H

#ifdef SERVERSSL
#include <openssl/ssl.h>

#include "BaseClassSwitch.h"

#define BUFFER_MAX_SIZE 4096

namespace CatchChallenger {
class EpollSslClient : public BaseClassSwitch
{
public:
    EpollSslClient(const int &infd, SSL_CTX *ctx);
    ~EpollSslClient();
    void close();
    ssize_t read(char *bufferClearToOutput,const size_t &bufferSizeClearToOutput);
    ssize_t write(const char *bufferClearToOutput, const size_t &bufferSizeClearToOutput);
    Type getType() const;
    bool isValid() const;
    long int bytesAvailable() const;
private:
    int infd;
    BIO* sbio;
    SSL* ssl;
    bool bHandShakeOver;
};
}

#endif

#endif // EPOLL_SSL_CLIENT_H
