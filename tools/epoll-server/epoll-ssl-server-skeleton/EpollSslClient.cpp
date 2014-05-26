#include "EpollSslClient.h"

#include <unistd.h>
#include <iostream>
#include <netinet/tcp.h>
#include <netdb.h>

char EpollSslClient::rawbuf[4096];

EpollSslClient::EpollSslClient(const int &infd,SSL_CTX *ctx) :
    #ifndef SERVERNOBUFFER
    bufferSizeClearToOutput(0),
    bufferSizeEncryptedToOutput(0),
    bufferSizeEncryptedToInput(0),
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
    memset(bufferClearToOutput,0,4096);
    memset(bufferEncryptedToOutput,0,4096);
    memset(bufferEncryptedToInput,0,4096);
    #endif

    SSL_set_bio(ssl, bioIn, bioOut);
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
    #ifndef SERVERNOBUFFER
    //flush the previous read
    if(bufferSizeEncryptedToInput>0)
    {
        const int &bufferUsed = BIO_write(bioIn, bufferEncryptedToInput, bufferSizeEncryptedToInput);
        if(bufferUsed>0)
        {
            if(bufferUsed!=(int)bufferSizeEncryptedToInput)
            {
                memmove(bufferEncryptedToInput,bufferEncryptedToInput+(bufferSizeEncryptedToInput-bufferUsed),bufferUsed);
                bufferSizeEncryptedToInput-=(bufferSizeEncryptedToInput-bufferUsed);
                //buffer full
                return -3;
            }
            else
                bufferSizeEncryptedToInput=0;
        }
        else
            return -3;
    }
    if(bufferSizeEncryptedToInput>=BUFFER_MAX_SIZE)
        return -5;
    int size_to_get=sizeof(rawbuf);
    if(size_to_get>(BUFFER_MAX_SIZE-(int)bufferSizeEncryptedToInput))
        size_to_get=(BUFFER_MAX_SIZE-bufferSizeEncryptedToInput);
    #else
    int size_to_get=sizeof(rawbuf);
    #endif
    const ssize_t &count=::read(infd, rawbuf, size_to_get);
    if(count == -1)
    {
        /* If errno == EAGAIN, that means we have read all
        data. So go back to the main loop. */
        if(errno != EAGAIN)
        {
            std::cerr << "Read socket error" << std::endl;
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
    if(bufferUsed!=count)
    {
        #ifndef SERVERNOBUFFER
        memcpy(bufferEncryptedToInput+bufferSizeEncryptedToInput,rawbuf+bufferUsed,(count-bufferUsed));
        #else
        return -5;
        #endif
    }
    else if(bufferUsed == -1 || bufferUsed == -2 || bufferUsed == 0)
    {
        //bug
        return -4;
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
            return 0;
        }
    }

    if(size != (int)bufferSize)
    {
        if(errno != EAGAIN)
        {
            std::cerr << "Socket buffer write error" << std::endl;
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
                    }
                    else
                    {
                        memcpy(this->bufferClearToOutput+this->bufferSizeClearToOutput,buffer,BUFFER_MAX_SIZE-this->bufferSizeClearToOutput);
                        this->bufferSizeClearToOutput=BUFFER_MAX_SIZE;
                    }
                }
                else
                {
                    const int &diff=bufferSize-size;
                    if((this->bufferSizeClearToOutput+diff)<BUFFER_MAX_SIZE)
                    {
                        memcpy(this->bufferClearToOutput+this->bufferSizeClearToOutput,buffer+size,diff);
                        this->bufferSizeClearToOutput+=bufferSize;
                    }
                    else
                    {
                        memcpy(this->bufferClearToOutput+this->bufferSizeClearToOutput,buffer+size,BUFFER_MAX_SIZE-this->bufferSizeClearToOutput);
                        this->bufferSizeClearToOutput=BUFFER_MAX_SIZE;
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
                #ifndef SERVERNOBUFFER
                if(this->bufferSizeEncryptedToOutput<BUFFER_MAX_SIZE)
                {
                    if(nRet<0)
                    {
                        if((this->bufferSizeEncryptedToOutput+bytesToSend)<BUFFER_MAX_SIZE)
                        {
                            memcpy(this->bufferEncryptedToOutput+this->bufferSizeEncryptedToOutput,rawbuf,bytesToSend);
                            this->bufferSizeEncryptedToOutput+=bytesToSend;
                        }
                        else
                        {
                            memcpy(this->bufferEncryptedToOutput+this->bufferSizeEncryptedToOutput,rawbuf,BUFFER_MAX_SIZE-this->bufferSizeEncryptedToOutput);
                            this->bufferSizeEncryptedToOutput=BUFFER_MAX_SIZE;
                        }
                    }
                    else
                    {
                        const int &diff=bytesToSend-nRet;
                        if((this->bufferSizeEncryptedToOutput+diff)<BUFFER_MAX_SIZE)
                        {
                            memcpy(this->bufferEncryptedToOutput+this->bufferSizeEncryptedToOutput,rawbuf+nRet,diff);
                            this->bufferSizeEncryptedToOutput+=bytesToSend;
                        }
                        else
                        {
                            memcpy(this->bufferEncryptedToOutput+this->bufferSizeEncryptedToOutput,rawbuf+nRet,BUFFER_MAX_SIZE-this->bufferSizeEncryptedToOutput);
                            this->bufferSizeEncryptedToOutput=BUFFER_MAX_SIZE;
                        }
                    }
                }
                #else
                std::cerr << "send() buffer full" << std::endl;
                return false;
                #endif
            }
        }
        else if (!BIO_should_retry(bioOut))
        {// BIO_should_retry() is true if the call that produced this condition should then be retried at a later time.
            const int32_t &ssl_error = SSL_get_error(ssl, bytesToSend);
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
        }
    }
    else
        std::cout << "BIO_ctrl_pending(bioOut) == 0" << std::endl;
    return true;
}

#ifndef SERVERNOBUFFER
void EpollSslClient::flush()
{
    if(bufferSizeClearToOutput>0)
    {
        size_t count=sizeof(rawbuf);
        if(bufferSizeClearToOutput<count)
            count=bufferSizeClearToOutput;
        memcpy(rawbuf,bufferClearToOutput,count);
        const ssize_t &size = SSL_write(ssl, rawbuf, count);
        if(size<0)
        {
            if(errno != EAGAIN)
            {
                std::cerr << "Ssl flush error" << std::endl;
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
    if(bufferSizeEncryptedToOutput>0)
    {
        size_t count=sizeof(rawbuf);
        count=sizeof(rawbuf);
        if(bufferSizeEncryptedToOutput<count)
            count=bufferSizeEncryptedToOutput;
        memcpy(rawbuf,bufferEncryptedToOutput,count);
        const ssize_t &size = ::write(infd, rawbuf, count);
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
                bufferSizeEncryptedToOutput-=(count-size);
                memmove(bufferEncryptedToOutput,bufferEncryptedToOutput+(count-size),bufferSizeEncryptedToOutput);
            }
            else
            {
                bufferSizeEncryptedToOutput-=size;
                memmove(bufferEncryptedToOutput,bufferEncryptedToOutput+size,bufferSizeEncryptedToOutput);
            }
        }
    }
}
#endif

BaseClassSwitch::Type EpollSslClient::getType() const
{
    return BaseClassSwitch::Type::Client;
}
