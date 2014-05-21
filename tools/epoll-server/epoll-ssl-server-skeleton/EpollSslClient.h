#ifndef CLIENT_H
#define CLIENT_H

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
    void close();
    ssize_t read(char *buffer,const size_t &bufferSize);
    ssize_t write(char *buffer,const size_t &bufferSize);
    void flush();
    void doRealWrite();
    Type getType() const;
private:
    static char rawbuf[BUFFER_MAX_SIZE];
private:
    char buffer[BUFFER_MAX_SIZE];
    size_t bufferSize;
    int infd;
    //decrypt pipe
    BIO* bioIn;
    //encrypt pipe
    BIO* bioOut;
    //ssl context
    SSL* ssl;
    bool bHandShakeOver;
};

#endif // CLIENT_H
