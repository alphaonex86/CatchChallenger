#include <sys/epoll.h>
#include <errno.h>
#include <iostream>
#include <list>
#include <sys/socket.h>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <unistd.h>

#include "Client.h"
#include "Socket.h"
#include "Server.h"
#include "Epoll.h"
#include "Timer.h"
#include "TimerDisplayEventBySeconds.h"

#define MAXEVENTS 512
#define MAXCLIENT 6000

//Graph: create SSL_METHOD and create SSL_CTX
SSL_CTX* InitServerCTX()
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
void LoadCertificates(SSL_CTX* ctx, const char* CertFile, const char* KeyFile)
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
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}

int main(int argc, char *argv[])
{
    SSL_library_init(); /* load encryption & hash algorithms for SSL */
    SSL_load_error_strings(); /* load the error strings for good error reporting */
    //Graph: create SSL_METHOD and create SSL_CTX
    SSL_CTX *ctx = InitServerCTX();        /* initialize SSL */
    //Graph: Load the certificat into SSL_CTX
    LoadCertificates(ctx, "/home/user/Desktop/CatchChallenger/test/openssl-server/server.crt", "/home/user/Desktop/CatchChallenger/test/openssl-server/server.key"); /* load certs */

    if(argc != 2)
    {
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if(!Epoll::epoll.init())
        abort();

    Server server;
    if(!server.tryListen(argv[1]))
        abort();
    TimerDisplayEventBySeconds timerDisplayEventBySeconds;
    if(!timerDisplayEventBySeconds.init())
        abort();

    char buf[4096];
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];

    Client* clients[MAXCLIENT];
    int clientListSize=0;
    /* The event loop */
    int number_of_events, i;
    while(1)
    {
        number_of_events = Epoll::epoll.wait(events, MAXEVENTS);
        for(i = 0; i < number_of_events; i++)
        {
            switch(static_cast<BaseClassSwitch *>(events[i].data.ptr)->getType())
            {
                case BaseClassSwitch::Type::Server:
                {
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        fprintf(stderr, "server epoll error\n");
                        continue;
                    }
                    /* We have a notification on the listening socket, which
                    means one or more incoming connections. */
                    while(1)
                    {
                        if(clientListSize>=MAXCLIENT)
                            break;

                        sockaddr in_addr;
                        socklen_t in_len = sizeof(in_addr);
                        const int &infd = server.accept(&in_addr, &in_len);

                        if(infd == -1)
                        {
                            if((errno == EAGAIN) ||
                            (errno == EWOULDBLOCK))
                            {
                                /* We have processed all incoming
                                connections. */
                                break;
                            }
                            else
                            {
                                perror("accept");
                                break;
                            }
                        }

                        //Graph: Create SSL
                        SSL *ssl = SSL_new(ctx);
                        //Graph create BIO:
                        BIO* bioIn = BIO_new(BIO_s_mem());
                        BIO* bioOut = BIO_new(BIO_s_mem());
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

                        //just for informations
                        {
                            char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
                            const int &s = getnameinfo(&in_addr, in_len,
                            hbuf, sizeof hbuf,
                            sbuf, sizeof sbuf,
                            NI_NUMERICHOST | NI_NUMERICSERV);
                            if(s == 0)
                                printf("Accepted connection on descriptor %d (host=%s, port=%s)\n", infd, hbuf, sbuf);
                        }

                        /* Make the incoming socket non-blocking and add it to the
                        list of fds to monitor. */
                        Client *client=new Client();
                        client->infd=infd;
                        //decrypt pipe
                        client->bioIn=bioIn;
                        //encrypt pipe
                        client->bioOut=bioOut;
                        client->ssl=ssl;
                        clients[clientListSize]=client;
                        clientListSize++;
                        int s = Socket::make_non_blocking(infd);
                        if(s == -1)
                            abort();
                        epoll_event event;
                        event.data.ptr = client;
                        event.events = EPOLLIN | EPOLLET | EPOLLOUT;
                        s = Epoll::epoll.ctl(EPOLL_CTL_ADD, infd, &event);
                        if(s == -1)
                        {
                            perror("epoll_ctl");
                            abort();
                        }
                    }
                    continue;
                }
                break;
                case BaseClassSwitch::Type::Client:
                {
                    timerDisplayEventBySeconds.addCount();
                    Client *client=static_cast<Client *>(events[i].data.ptr);
                    if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        fprintf(stderr, "client epoll error\n");
                        client->close();
                        continue;
                    }
                    //ready to read
                    if(events[i].events & EPOLLIN)
                    {
                        /* We have data on the fd waiting to be read. Read and
                        display it. We must read whatever data is available
                        completely, as we are running in edge-triggered mode
                        and won't get a notification again for the same
                        data. */
                        while (1)
                        {
                            const ssize_t &count = client->read(buf,sizeof(buf));
                            if(count==-2)
                            {
                                //disconnect, free the ressources
                                SSL_free(client->ssl);
                                //replace by delete?
                                client->bioIn=NULL;
                                client->bioOut=NULL;
                                client->ssl=NULL;
                                client->bHandShakeOver=false;
                                break;
                            }
                            //write the encrypted data into decrypt pipe
                            int bufferUsed = BIO_write(client->bioIn, buf, count);
                            if(bufferUsed == -1 || bufferUsed == -2 || bufferUsed == 0)
                            {
                                // error
                            }
                            //read clear data from the decrypt pipe
                            int bytesOut = SSL_read(client->ssl, (void*)buf, sizeof(buf));
                            if(bytesOut > 0)
                            {
                                std::cout << "decoded byte: " << bytesOut << std::endl;
                                //write clear data to encrypt pipe
                                int bytesToWrite = SSL_write(client->ssl, (void*)buf, bytesOut);
                                if(bytesToWrite > 0)
                                    std::cout << "reemited byte: " << bytesOut << std::endl;
                                else
                                {
                                    if (SSL_want_write(client->ssl))
                                    {
                                        std::cout << "SSL_want_write" << std::endl;
                                    }
                                    else
                                    {
                                        int32_t ssl_error = SSL_get_error(client->ssl, bytesOut);
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
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                if (SSL_want_read(client->ssl))
                                {
                                    std::cout << "SSL_want_read" << std::endl;
                                }
                                else
                                {
                                    int32_t ssl_error = SSL_get_error(client->ssl, bytesOut);
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
                                    break;
                                }

                                if(!client->bHandShakeOver && SSL_is_init_finished(client->ssl))
                                {
                                    //Graph: Finish the handshake
                                    std::cout << "Handshake has been finished" << std::endl;
                                    client->bHandShakeOver = true;

                                    char cipdesc[128];
                                    const SSL_CIPHER* sc = SSL_get_current_cipher(client->ssl);
                                    if (sc)
                                        std::cout << "encryption: " << SSL_CIPHER_description(sc, cipdesc, sizeof(cipdesc)) << std::endl;
                                }
                            }
                            while (1)
                            {
                                // BIO_ctrl_pending() returns the number of bytes buffered in a BIO.
                                size_t pending = BIO_ctrl_pending(client->bioOut);
                                if (pending > 0)
                                {
                                    std::cout << "BIO_ctrl_pending(bioOut) == " << pending << std::endl;

                                    //  BIO_read() attempts to read len bytes from BIO b and places the data in buf.
                                    int bytesToSend = BIO_read(client->bioOut, (void*)buf, sizeof(buf) > pending ? pending : sizeof(buf));
                                    if (bytesToSend > 0)
                                    {
                                        std::cout << "BIO_read(bioOut) == " << bytesToSend << std::endl;

                                        //write the encrypted data from the encrypt pipe
                                        ssize_t nRet = client->write(buf, bytesToSend);
                                        if(nRet<0)
                                        {
                                            //bReplyOver = true;
                                            std::cerr << "send() - SOCKET_ERROR" << std::endl;
                                        }
                                    }
                                    else if (!BIO_should_retry(client->bioOut))
                                    {// BIO_should_retry() is true if the call that produced this condition should then be retried at a later time.
                                        int32_t ssl_error = SSL_get_error(client->ssl, bytesOut);
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
                                {
                                    std::cout << "BIO_ctrl_pending(bioOut) == 0" << std::endl;
                                    break;
                                }
                            }

                            /*if (bReplyOver)
                            {
                                std::cout << "post-reply" << std::endl;
                                break;
                            }*/
                            if(count>0)
                            {
                                //broadcast to all the client
                                /*int index=0;
                                while(index<clientListSize)
                                {
                                    clients[index]->write(buf,count);
                                    index++;
                                }*/
                            }
                            else
                                break;
                        }
                    }
                    //ready to write
                    if(events[i].events & EPOLLOUT)
                    {
                        client->flush();
                    }
                }
                break;
                case BaseClassSwitch::Type::Timer:
                    static_cast<Timer *>(events[i].data.ptr)->exec();
                break;
                default:
                    fprintf(stderr, "unknown event\n");
                break;
            }
        }
    }
    server.close();
    return EXIT_SUCCESS;
}
