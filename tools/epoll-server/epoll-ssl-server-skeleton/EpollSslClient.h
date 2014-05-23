#ifndef EPOLL_SSL_CLIENT_H
#define EPOLL_SSL_CLIENT_H

#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#include "BaseClassSwitch.h"

#define BUFFER_MAX_SIZE 4096

class EpollSslClient : public BaseClassSwitch
{
public:
    EpollSslClient(const int &infd,SSL_CTX *ctx);
    ~EpollSslClient();
    #ifndef SERVERNOBUFFER
    static void staticInit();
    #endif
    void close();
    ssize_t read(char *buffer,const size_t &bufferSize);
    ssize_t write(char *buffer,const size_t &bufferSize);
    #ifndef SERVERNOBUFFER
    void flush();
    #endif
    bool doRealWrite();
    Type getType() const;
private:
    static char rawbuf[BUFFER_MAX_SIZE];
private:
    #ifndef SERVERNOBUFFER
    char buffer[BUFFER_MAX_SIZE];
    size_t bufferSize;
    #endif
    int infd;
    //decrypt pipe
    BIO* bioIn;
    //encrypt pipe
    BIO* bioOut;
    //ssl context
    SSL* ssl;
    bool bHandShakeOver;
};

#endif // EPOLL_SSL_CLIENT_H
