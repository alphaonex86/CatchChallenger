#ifndef EPOLL_SSL_CLIENT_H
#define EPOLL_SSL_CLIENT_H

#ifndef SERVERNOSSL
#include <openssl/ssl.h>

#include "BaseClassSwitch.h"

#define BUFFER_MAX_SIZE 4096

namespace CatchChallenger {
class EpollSslClient : public BaseClassSwitch
{
public:
    EpollSslClient(const int &infd, SSL_CTX *ctx, const bool &tcpCork);
    ~EpollSslClient();
    #ifndef SERVERNOBUFFER
    static void staticInit();
    #endif
    void close();
    ssize_t read(char *bufferClearToOutput,const size_t &bufferSizeClearToOutput);
    ssize_t write(char *bufferClearToOutput,const size_t &bufferSizeClearToOutput);
    #ifndef SERVERNOBUFFER
    void flush();
    #endif
    Type getType() const;
    bool isValid() const;
    long int bytesAvailable() const;
private:
    static char rawbuf[BUFFER_MAX_SIZE];
private:
    #ifndef SERVERNOBUFFER
    char bufferClearToOutput[BUFFER_MAX_SIZE];
    size_t bufferSizeClearToOutput;
    #endif
    int infd;
    BIO* sbio;
    SSL* ssl;
    bool bHandShakeOver;
};
}

#endif

#endif // EPOLL_SSL_CLIENT_H
