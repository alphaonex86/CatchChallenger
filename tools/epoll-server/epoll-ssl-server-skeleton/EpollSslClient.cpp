#ifndef SERVERNOSSL

#include "EpollSslClient.h"

#include <unistd.h>
#include <iostream>
#include <netinet/tcp.h>
#include <netdb.h>

char EpollSslClient::rawbuf[4096];

EpollSslClient::EpollSslClient(const int &infd,SSL_CTX *ctx) :
    #ifndef SERVERNOBUFFER
    bufferSizeClearToOutput(0),
    #endif
    infd(infd),
    //ssl context
    ssl(SSL_new(ctx)),
    bHandShakeOver(false)
{
    ssl = SSL_new(ctx);
    sbio=BIO_new(BIO_s_socket());
    BIO_set_fd(sbio, infd, BIO_NOCLOSE);
    SSL_set_bio(ssl, sbio, sbio);
    bHandShakeOver=false;
    #ifndef SERVERNOBUFFER
    memset(bufferClearToOutput,0,4096);
    #endif

    //Graph: Start the handshake
    int err = SSL_accept(ssl);
    int32_t ssl_error = SSL_get_error(ssl, err);
    switch (ssl_error) {
        case SSL_ERROR_NONE:
        std::cout << "SSL_ERROR_NONE" << std::endl;
        break;
        case SSL_ERROR_WANT_READ:
        std::cout << "SSL_ERROR_WANT_READ" << std::endl;
        break;
        case SSL_ERROR_WANT_WRITE:
        std::cout << "SSL_ERROR_WANT_WRITE" << std::endl;
        break;
        case SSL_ERROR_ZERO_RETURN:
        std::cout << "SSL_ERROR_ZERO_RETURN" << std::endl;
        break;
        default:
        break;
    }

    //set cork for CatchChallener because don't have real time part
    int state = 1;
    if(setsockopt(infd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
        std::cerr << "Unable to apply tcp cork" << std::endl;
}

EpollSslClient::~EpollSslClient()
{
    close();
}

#ifndef SERVERNOBUFFER
void EpollSslClient::staticInit()
{
    memset(rawbuf,0,4096);
}
#endif

void EpollSslClient::close()
{
    #ifndef SERVERNOBUFFER
    bufferSizeClearToOutput=0;
    #endif
    if(infd!=-1)
    {
        /* Closing the descriptor will make epoll remove it
        from the set of descriptors which are monitored. */
        ::close(infd);
        std::cout << "Closed connection on descriptor" << infd << std::endl;
        infd=-1;

        //disconnect, free the ressources
        SSL_free(ssl);
        //replace by delete?
        sbio=NULL;
        ssl=NULL;
        bHandShakeOver=false;
    }
}

ssize_t EpollSslClient::read(char *buffer,const size_t &bufferSize)
{
    if(infd==-1)
        return -1;
    return SSL_read(ssl, buffer, bufferSize);
}

ssize_t EpollSslClient::write(char *buffer,const size_t &bufferSize)
{
    if(infd==-1)
        return -1;
    const int &size=SSL_write(ssl, buffer, bufferSize);
    if(size != (ssize_t)bufferSize)
    {
        if(errno != EAGAIN)
        {
            std::cerr << "Write socket error" << std::endl;
            close();
            return -1;
        }
        else
        {
            #ifndef SERVERNOBUFFER
            if(this->bufferSizeClearToOutput<BUFFER_MAX_SIZE)
            {
                if(size<0)
                {
                    if((this->bufferSizeClearToOutput+bufferSize)<BUFFER_MAX_SIZE)
                    {
                        memcpy(this->bufferClearToOutput+this->bufferSizeClearToOutput,buffer,bufferSize);
                        this->bufferSizeClearToOutput+=bufferSize;
                        return bufferSize;
                    }
                    else
                    {
                        memcpy(this->bufferClearToOutput+this->bufferSizeClearToOutput,buffer,BUFFER_MAX_SIZE-this->bufferSizeClearToOutput);
                        this->bufferSizeClearToOutput=BUFFER_MAX_SIZE;
                        return BUFFER_MAX_SIZE-this->bufferSizeClearToOutput;
                    }
                }
                else
                {
                    const int &diff=bufferSize-size;
                    if((this->bufferSizeClearToOutput+diff)<BUFFER_MAX_SIZE)
                    {
                        memcpy(this->bufferClearToOutput+this->bufferSizeClearToOutput,buffer+size,diff);
                        this->bufferSizeClearToOutput+=bufferSize;
                        return bufferSize;
                    }
                    else
                    {
                        memcpy(this->bufferClearToOutput+this->bufferSizeClearToOutput,buffer+size,BUFFER_MAX_SIZE-this->bufferSizeClearToOutput);
                        this->bufferSizeClearToOutput=BUFFER_MAX_SIZE;
                        return BUFFER_MAX_SIZE-this->bufferSizeClearToOutput;
                    }
                }
            }
            #endif
            return size;
        }
    }
    else
        return size;
}

#ifndef SERVERNOBUFFER
void EpollSslClient::flush()
{
    if(bufferSizeClearToOutput>0)
    {
        size_t count=sizeof(rawbuf);
        count=sizeof(rawbuf);
        if(bufferSizeClearToOutput<count)
            count=bufferSizeClearToOutput;
        memcpy(rawbuf,bufferClearToOutput,count);
        const ssize_t &size = SSL_write(ssl, rawbuf, count);
        if(size<0)
        {
            if(errno != EAGAIN)
            {
                std::cerr << "Socket buffer flush error" << std::endl;
                close();
            }
        }
        else
        {
            if(size!=(int)count)
            {
                bufferSizeClearToOutput-=(count-size);
                memmove(bufferClearToOutput,bufferClearToOutput+(count-size),bufferSizeClearToOutput);
            }
            else
            {
                bufferSizeClearToOutput-=size;
                memmove(bufferClearToOutput,bufferClearToOutput+size,bufferSizeClearToOutput);
            }
        }
    }
}
#endif

BaseClassSwitch::Type EpollSslClient::getType() const
{
    return BaseClassSwitch::Type::Client;
}
#endif
