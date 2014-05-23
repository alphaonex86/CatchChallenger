#include "EpollSslClient.h"

#include <unistd.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

char EpollSslClient::rawbuf[4096];

EpollSslClient::EpollSslClient(const int &infd,SSL_CTX *ctx) :
    #ifndef SERVERNOBUFFER
    bufferSize(0),
    #endif
    infd(infd),
    //decrypt pipe
    bioIn(BIO_new(BIO_s_mem())),
    //encrypt pipe
    bioOut(BIO_new(BIO_s_mem())),
    //ssl context
    ssl(SSL_new(ctx)),
    bHandShakeOver(false)
{
    bHandShakeOver=false;
    #ifndef SERVERNOBUFFER
    bufferSize=0;
    memset(buffer,0,4096);
    #endif

    SSL_set_bio(ssl, bioIn, bioOut);
    //Graph: Start the handshake
    int err = SSL_accept(ssl);
    int32_t ssl_error = SSL_get_error(ssl, err);
    switch (ssl_error) {
        case SSL_ERROR_NONE:
        printf("SSL_ERROR_NONE\n");
        break;
        case SSL_ERROR_WANT_READ:
        printf("SSL_ERROR_WANT_READ\n");
        break;
        case SSL_ERROR_WANT_WRITE:
        printf("SSL_ERROR_WANT_WRITE\n");
        break;
        case SSL_ERROR_ZERO_RETURN:
        printf("SSL_ERROR_ZERO_RETURN\n");
        break;
        default:
        break;
    }

    //set cork for CatchChallener because don't have real time part
    int state = 1;
    if(setsockopt(infd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
        perror("Unable to apply tcp cork");
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
    bufferSize=0;
    #endif
    if(infd!=-1)
    {
        /* Closing the descriptor will make epoll remove it
        from the set of descriptors which are monitored. */
        ::close(infd);
        printf("Closed connection on descriptor %d\n",infd);
        infd=-1;

        //disconnect, free the ressources
        SSL_free(ssl);
        //replace by delete?
        bioIn=NULL;
        bioOut=NULL;
        ssl=NULL;
        bHandShakeOver=false;
    }
}

ssize_t EpollSslClient::read(char *buffer,const size_t &bufferSize)
{
    if(infd==-1)
        return -1;
    const ssize_t &count=::read(infd, rawbuf, sizeof(rawbuf));
    if(count == -1)
    {
        /* If errno == EAGAIN, that means we have read all
        data. So go back to the main loop. */
        if(errno != EAGAIN)
        {
            perror("read");
            close();
            return -1;
        }
    }
    else if(count == 0)
    {
        /* End of file. The remote has closed the
        connection. */
        close();
        return -2;
    }

    //write the encrypted data into decrypt pipe
    int bufferUsed = BIO_write(bioIn, rawbuf, count);
    if(bufferUsed == -1 || bufferUsed == -2 || bufferUsed == 0)
    {
        // error
    }
    //read clear data from the decrypt pipe
    const int &bytesOut = SSL_read(ssl, static_cast<void*>(buffer), sizeof(bufferSize));
    if(bytesOut > 0)
    {
        std::cout << "decoded byte: " << bytesOut << std::endl;
        return bytesOut;
    }
    else
    {
        if(SSL_want_read(ssl))
        {
            std::cout << "SSL_want_read" << std::endl;
        }
        else
        {
            int32_t ssl_error = SSL_get_error(ssl, bytesOut);
            switch (ssl_error) {
                case SSL_ERROR_NONE:
                printf("SSL_ERROR_NONE\n");
                break;
                case SSL_ERROR_WANT_READ:
                printf("SSL_ERROR_WANT_READ\n");
                break;
                case SSL_ERROR_WANT_WRITE:
                printf("SSL_ERROR_WANT_WRITE\n");
                break;
                case SSL_ERROR_ZERO_RETURN:
                printf("SSL_ERROR_ZERO_RETURN\n");
                break;
                default:
                break;
            }
            return 0;
        }

        if(!bHandShakeOver && SSL_is_init_finished(ssl))
        {
            //Graph: Finish the handshake
            std::cout << "Handshake has been finished" << std::endl;
            bHandShakeOver = true;

            char cipdesc[128];
            const SSL_CIPHER* sc = SSL_get_current_cipher(ssl);
            if (sc)
                std::cout << "encryption: " << SSL_CIPHER_description(sc, cipdesc, sizeof(cipdesc)) << std::endl;
        }
    }

    return 0;
}

ssize_t EpollSslClient::write(char *buffer,const size_t &bufferSize)
{
    if(infd==-1)
        return -1;
    //write clear data to encrypt pipe
    const int &size = SSL_write(ssl, (void*)buffer, bufferSize);
    if(size > 0)
        std::cout << "reemited byte: " << size << std::endl;
    else
    {
        if (SSL_want_write(ssl))
        {
            std::cout << "SSL_want_write" << std::endl;
        }
        else
        {
            int32_t ssl_error = SSL_get_error(ssl, size);
            switch (ssl_error) {
                case SSL_ERROR_NONE:
                printf("SSL_ERROR_NONE\n");
                break;
                case SSL_ERROR_WANT_READ:
                printf("SSL_ERROR_WANT_READ\n");
                break;
                case SSL_ERROR_WANT_WRITE:
                printf("SSL_ERROR_WANT_WRITE\n");
                break;
                case SSL_ERROR_ZERO_RETURN:
                printf("SSL_ERROR_ZERO_RETURN\n");
                break;
                default:
                break;
            }
            return 0;
        }
    }

    //const ssize_t &size = ::write(infd, buffer, bufferSize);
    if(size != (int)bufferSize)
    {
        if(errno != EAGAIN)
        {
            perror("write");
            close();
            return -1;
        }
        else
        {
            #ifndef SERVERNOBUFFER
            if(this->bufferSize<BUFFER_MAX_SIZE)
            {
                if(size<0)
                {
                    if((this->bufferSize+bufferSize)<BUFFER_MAX_SIZE)
                    {
                        memcpy(this->buffer+this->bufferSize,buffer,bufferSize);
                        this->bufferSize+=bufferSize;
                    }
                    else
                    {
                        memcpy(this->buffer+this->bufferSize,buffer,BUFFER_MAX_SIZE-this->bufferSize);
                        this->bufferSize=BUFFER_MAX_SIZE;
                    }
                }
                else
                {
                    const int &diff=bufferSize-size;
                    if((this->bufferSize+diff)<BUFFER_MAX_SIZE)
                    {
                        memcpy(this->buffer+this->bufferSize,buffer+size,diff);
                        this->bufferSize+=bufferSize;
                    }
                    else
                    {
                        memcpy(this->buffer+this->bufferSize,buffer+size,BUFFER_MAX_SIZE-this->bufferSize);
                        this->bufferSize=BUFFER_MAX_SIZE;
                    }
                }
            }
            #endif
            if(!doRealWrite())
                return -3;
            return size;
        }
    }
    else
    {
        if(!doRealWrite())
            return -3;
        return size;
    }
}

bool EpollSslClient::doRealWrite()
{
    // BIO_ctrl_pending() returns the number of bytes buffered in a BIO.
    const size_t &pending = BIO_ctrl_pending(bioOut);
    if(pending > 0)
    {
        std::cout << "BIO_ctrl_pending(bioOut) == " << pending << std::endl;

        //  BIO_read() attempts to read len bytes from BIO b and places the data in buf.
        const int &bytesToSend = BIO_read(bioOut, (void*)rawbuf, sizeof(rawbuf) > pending ? pending : sizeof(rawbuf));
        if(bytesToSend>0)
        {
            std::cout << "BIO_read(bioOut) == " << bytesToSend << std::endl;

            //write the encrypted data from the encrypt pipe
            const ssize_t &nRet = ::write(infd,rawbuf, bytesToSend);
            if(nRet<0)
            {
                //bReplyOver = true;
                std::cerr << "send() - SOCKET_ERROR" << std::endl;
                return false;
            }
            if(nRet!=bytesToSend)
            {
                std::cerr << "send() buffer full" << std::endl;
                return false;
            }
        }
        else if (!BIO_should_retry(bioOut))
        {// BIO_should_retry() is true if the call that produced this condition should then be retried at a later time.
            const int32_t &ssl_error = SSL_get_error(ssl, bytesToSend);
            switch (ssl_error) {
                case SSL_ERROR_NONE:
                printf("SSL_ERROR_NONE\n");
                break;
                case SSL_ERROR_WANT_READ:
                printf("SSL_ERROR_WANT_READ\n");
                break;
                case SSL_ERROR_WANT_WRITE:
                printf("SSL_ERROR_WANT_WRITE\n");
                break;
                case SSL_ERROR_ZERO_RETURN:
                printf("SSL_ERROR_ZERO_RETURN\n");
                break;
                default:
                break;
            }
        }
    }
    else
        std::cout << "BIO_ctrl_pending(bioOut) == 0" << std::endl;
    return true;
}

#ifndef SERVERNOBUFFER
void EpollSslClient::flush()
{
    if(bufferSize>0)
    {
        size_t count;
        count=sizeof(rawbuf);
        if(bufferSize<count)
            count=bufferSize;
        memcpy(rawbuf,buffer,count);
        ssize_t size = SSL_write(ssl, rawbuf, count);//::write(infd, buf, count);
        if(size<0)
        {
            if(errno != EAGAIN)
            {
                perror("write");
                close();
            }
        }
        else
        {
            bufferSize-=size;
            memmove(buffer,buffer+size,bufferSize);
        }
    }
    doRealWrite();
}
#endif

BaseClassSwitch::Type EpollSslClient::getType() const
{
    return BaseClassSwitch::Type::Client;
}
