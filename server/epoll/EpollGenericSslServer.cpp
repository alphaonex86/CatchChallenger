#ifdef SERVERSSL

#include "EpollGenericSslServer.h"

#include <openssl/err.h>
#include <iostream>

using namespace CatchChallenger;

EpollGenericSslServer::EpollGenericSslServer()
{
}

bool EpollGenericSslServer::trySslListen(const char* const ip,const char* const port,const char* const CertFile, const char* const KeyFile)
{
    if(!EpollGenericServer::tryListenInternal(ip,port))
        return false;

    initSslPart(CertFile,KeyFile);
    return true;
}

void EpollGenericSslServer::initSslPart(const char* const CertFile, const char* const KeyFile)
{
    SSL_library_init(); /* load encryption & hash algorithms for SSL */
    SSL_load_error_strings(); /* load the error strings for good error reporting */
    //Graph: create SSL_METHOD and create SSL_CTX
    ctx = InitServerCTX();        /* initialize SSL */
    //Graph: Load the certificat into SSL_CTX
    LoadCertificates(ctx, CertFile, KeyFile); /* load certs */
}

SSL_CTX * EpollGenericSslServer::getCtx() const
{
    return ctx;
}

//Graph: create SSL_METHOD and create SSL_CTX
SSL_CTX* EpollGenericSslServer::InitServerCTX()
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
void EpollGenericSslServer::LoadCertificates(SSL_CTX* ctx, const char* const CertFile, const char* const KeyFile)
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

#endif
