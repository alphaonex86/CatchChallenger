#ifndef CLIENT_H
#define CLIENT_H

#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#include "BaseClassSwitch.h"

#define BUFFER_MAX_SIZE 4096

class Client : public BaseClassSwitch
{
public:
    Client();
    ~Client();
    void close();
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
    ssize_t read(char *buffer,const size_t &bufferSize);
    ssize_t write(char *buffer,const size_t &bufferSize);
    void flush();
    Type getType();
};

#endif // CLIENT_H
