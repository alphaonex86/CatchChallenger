#ifndef SERVERNOSSL

#include "EpollSslServer.h"
#include "EpollSocket.h"
#include "Epoll.h"

#include "../base/ServerStructures.h"
#include "../base/GlobalServerData.h"

#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include <openssl/err.h>

using namespace CatchChallenger;

void eventLoop();

EpollSslServer::EpollSslServer()
{
    ready=false;
    sfd=-1;

    normalServerSettings.server_ip      = QString();
    normalServerSettings.server_port    = 42489;
    normalServerSettings.useSsl         = true;
    #ifdef Q_OS_LINUX
    normalServerSettings.linuxSettings.tcpCork                      = true;
    #endif
}

EpollSslServer::~EpollSslServer()
{
    close();
}

void EpollServer::preload_finish()
{
    ready=true;
}

bool EpollServer::isReady()
{
    return ready;
}

//Graph: create SSL_METHOD and create SSL_CTX
SSL_CTX* EpollSslServer::InitServerCTX()
{
    OpenSSL_add_all_algorithms();  /* load & register all encryptos, etc. */
    SSL_load_error_strings();   /* load all error messages */
    const SSL_METHOD *method = TLSv1_2_server_method();  /* create new server-method instance */
    SSL_CTX *ctx = SSL_CTX_new(method);   /* create new context from method */
    if(ctx == NULL)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

//Graph: Load the certificat into SSL_CTX
void EpollSslServer::LoadCertificates(SSL_CTX* ctx, const char* CertFile, const char* KeyFile)
{

    SSL_CTX_set_verify(ctx,SSL_VERIFY_NONE,NULL);
    //no ca cert because I use self signed certicat
    //SSL_CTX_load_verify_locations(ctx,"/home/user/cacert.pem",NULL);
    //no compression because CatchChallenger have their own compression, and add lot of bandwith on small packet
    SSL_CTX_set_options(ctx, SSL_OP_NO_COMPRESSION);

    /* set the local certificate from CertFile */
    if(SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM)<=0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if(SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM)<=0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if(!SSL_CTX_check_private_key(ctx))
    {
        std::cerr << "Private key does not match the public certificate" << std::endl;
        abort();
    }
}

bool EpollSslServer::tryListen()
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s;

    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;     /* All interfaces */

    if(!normalServerSettings.server_ip.isEmpty())
        s = getaddrinfo(normalServerSettings.server_ip.toUtf8().constData(), QString::number(normalServerSettings.server_port).toUtf8().constData(), &hints, &result);
    else
        s = getaddrinfo(NULL, QString::number(normalServerSettings.server_port).toUtf8().constData(), &hints, &result);
    if (s != 0)
    {
        std::cerr << "getaddrinfo:" << gai_strerror(s) << std::endl;
        return false;
    }

    for(rp = result; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sfd == -1)
        continue;

        s = bind (sfd, rp->ai_addr, rp->ai_addrlen);
        if (s == 0)
        {
            /* We managed to bind successfully! */
            break;
        }

        ::close(sfd);
    }

    if(rp == NULL)
    {
        sfd=-1;
        std::cerr << "Could not bind" << std::endl;
        return false;
    }

    freeaddrinfo (result);

    s = EpollSocket::make_non_blocking(sfd);
    if(s == -1)
    {
        sfd=-1;
        std::cerr << "Can't put in non blocking" << std::endl;
        return false;
    }
    yes=1;
    if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)))
    {
        sfd=-1;
        std::cerr << "Can't put in reuse" << std::endl;
        return false;
    }

    s = listen(sfd, SOMAXCONN);
    if(s == -1)
    {
        sfd=-1;
        std::cerr << "Listen error" << std::endl;
        return false;
    }

    epoll_event event;
    event.data.ptr = this;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    s = Epoll::epoll.ctl(EPOLL_CTL_ADD, sfd, &event);
    if(s == -1)
    {
        sfd=-1;
        std::cerr << "epoll_ctl error" << std::endl;
        return false;
    }

    initSslPart();
    return true;
}

void EpollSslServer::initSslPart()
{
    SSL_library_init(); /* load encryption & hash algorithms for SSL */
    SSL_load_error_strings(); /* load the error strings for good error reporting */
    //Graph: create SSL_METHOD and create SSL_CTX
    ctx = InitServerCTX();        /* initialize SSL */
    //Graph: Load the certificat into SSL_CTX
    LoadCertificates(ctx, "server.crt", "server.key"); /* load certs */
}

void EpollSslServer::close()
{
    if(sfd!=-1)
        ::close(sfd);
}

int EpollSslServer::accept(sockaddr *in_addr,socklen_t *in_len)
{
    return ::accept(sfd, in_addr, in_len);
}

int EpollSslServer::getSfd()
{
    return sfd;
}

BaseClassSwitch::Type EpollSslServer::getType() const
{
    return BaseClassSwitch::Type::Server;
}

SSL_CTX * EpollSslServer::getCtx() const
{
    return ctx;
}

void EpollSslServer::preload_the_data()
{
    BaseServer::preload_the_data();
}

CatchChallenger::ClientMapManagement * EpollSslServer::getClientMapManagement()
{
    return BaseServer::getClientMapManagement();
}

void EpollSslServer::unload_the_data()
{
    BaseServer::unload_the_data();
}


void EpollSslServer::setNormalSettings(const NormalServerSettings &settings)
{
    normalServerSettings=settings;
    loadAndFixSettings();
}

NormalServerSettings EpollSslServer::getNormalSettings() const
{
    return normalServerSettings;
}

void EpollSslServer::loadAndFixSettings()
{
    if(normalServerSettings.server_port<=0)
        normalServerSettings.server_port=42489;
    if(normalServerSettings.proxy_port<=0)
        normalServerSettings.proxy=QString();
}
#endif
