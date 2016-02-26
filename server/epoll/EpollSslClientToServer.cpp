#ifdef SERVERSSL

#include "EpollSslClientToServer.h"

#include <unistd.h>
#include <iostream>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include "../base/GlobalServerData.h"
#include "Epoll.h"
#include "EpollSocket.h"

using namespace CatchChallenger;

EpollSslClientToServer::EpollSslClientToServer(const int &infd,SSL_CTX *ctx) :
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
}

EpollSslClientToServer::~EpollSslClientToServer()
{
    close();
}

void EpollSslClientToServer::close()
{
    if(infd!=-1)
    {
        /* Closing the descriptor will make epoll remove it
        from the set of descriptors which are monitored. */
        Epoll::epoll.ctl(EPOLL_CTL_DEL, infd, NULL);
        ::close(infd);
        //std::cout << "Closed connection on descriptor" << infd << std::endl;
        infd=-1;

        //disconnect, free the ressources
        SSL_free(ssl);
        //replace by delete?
        sbio=NULL;
        ssl=NULL;
        bHandShakeOver=false;
    }
}

ssize_t EpollSslClientToServer::read(char *buffer,const size_t &bufferSize)
{
    if(infd==-1)
        return -1;
    return SSL_read(ssl, buffer, bufferSize);
}

ssize_t EpollSslClientToServer::write(const char *buffer,const size_t &bufferSize)
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
            std::cerr << "Write socket full: EAGAIN for size:" << bufferSize << std::endl;
            return size;
        }
    }
    else
        return size;
}

BaseClassSwitch::Type EpollSslClientToServer::getType() const
{
    return BaseClassSwitch::Type::Client;
}

bool EpollSslClientToServer::isValid() const
{
    return infd!=-1;
}

long int EpollSslClientToServer::bytesAvailable() const
{
    if(infd==-1)
        return -1;
    unsigned long int nbytes;
    // gives shorter than true amounts on Unix domain sockets.
    if(ioctl(infd, FIONREAD, &nbytes)>=0)
        return nbytes;
    else
        return -1;
}
#endif
