#include "CliSocket.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace CatchChallenger;

CliSocket::CliSocket() :
    socketFd(-1),
    connecting(false),
    errorString()
{
}

CliSocket::~CliSocket()
{
    closeSocket();
}

bool CliSocket::applySocketOptions()
{
    const int flags=::fcntl(socketFd,F_GETFL,0);
    if(flags<0)
    {
        errorString=std::string("fcntl(F_GETFL) failed: ")+strerror(errno);
        return false;
    }
    if(::fcntl(socketFd,F_SETFL,flags|O_NONBLOCK)<0)
    {
        errorString=std::string("fcntl(F_SETFL,O_NONBLOCK) failed: ")+strerror(errno);
        return false;
    }
    //latency matters more than packet count for a benchmark client: the
    //protocol is request/reply, Nagle would batch the login handshake.
    const int one=1;
    if(::setsockopt(socketFd,IPPROTO_TCP,TCP_NODELAY,&one,sizeof(one))<0)
    {
        //not fatal, only slower: report and keep going
        std::cerr << "CliSocket: setsockopt(TCP_NODELAY) failed: " << strerror(errno) << std::endl;
    }
    return true;
}

bool CliSocket::startConnect(const std::string &host,const uint16_t &port)
{
    closeSocket();
    errorString.clear();

    struct addrinfo hints;
    memset(&hints,0,sizeof(hints));
    hints.ai_family=AF_UNSPEC;//IPv4 + IPv6
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_protocol=IPPROTO_TCP;

    struct addrinfo *resolved=NULL;
    const std::string portString=std::to_string(port);
    const int resolveCode=::getaddrinfo(host.c_str(),portString.c_str(),&hints,&resolved);
    if(resolveCode!=0)
    {
        errorString="getaddrinfo("+host+":"+portString+") failed: "+gai_strerror(resolveCode);
        return false;
    }

    struct addrinfo *cursor=resolved;
    while(cursor!=NULL)
    {
        socketFd=::socket(cursor->ai_family,cursor->ai_socktype,cursor->ai_protocol);
        if(socketFd<0)
            errorString=std::string("socket() failed: ")+strerror(errno);
        else
        {
            if(!applySocketOptions())
            {
                ::close(socketFd);
                socketFd=-1;
            }
            else
            {
                if(::connect(socketFd,cursor->ai_addr,cursor->ai_addrlen)==0)
                {
                    //connect() CAN succeed at once (loopback, or a cached
                    //route). Stay in the "connecting" state anyway so the
                    //event loop always completes the handshake through the
                    //writable path: an already-connected fd is immediately
                    //writable, so select() returns without delay, and
                    //finishConnect() just reads SO_ERROR==0. Without this the
                    //fd would land in the READ set and readForFirstHeader()
                    //would never be called -> the bot hangs forever.
                    connecting=true;
                    ::freeaddrinfo(resolved);
                    return true;
                }
                else
                {
                    if(errno==EINPROGRESS || errno==EINTR)
                    {
                        connecting=true;
                        ::freeaddrinfo(resolved);
                        return true;
                    }
                    else
                    {
                        errorString=std::string("connect() failed: ")+strerror(errno);
                        ::close(socketFd);
                        socketFd=-1;
                    }
                }
            }
        }
        cursor=cursor->ai_next;
    }
    ::freeaddrinfo(resolved);
    if(errorString.empty())
        errorString="no usable address for "+host+":"+portString;
    return false;
}

bool CliSocket::finishConnect()
{
    if(socketFd<0)
    {
        errorString="finishConnect() on a closed socket";
        return false;
    }
    int socketError=0;
    socklen_t socketErrorSize=sizeof(socketError);
    if(::getsockopt(socketFd,SOL_SOCKET,SO_ERROR,&socketError,&socketErrorSize)<0)
    {
        errorString=std::string("getsockopt(SO_ERROR) failed: ")+strerror(errno);
        return false;
    }
    if(socketError!=0)
    {
        errorString=std::string("connect() failed: ")+strerror(socketError);
        return false;
    }
    connecting=false;
    return true;
}

bool CliSocket::isConnecting() const
{
    return connecting;
}

bool CliSocket::isValid() const
{
    return socketFd>=0;
}

int CliSocket::fd() const
{
    return socketFd;
}

ssize_t CliSocket::readData(char *data,const size_t &size)
{
    if(socketFd<0)
        return -1;
    while(true)
    {
        const ssize_t readSize=::recv(socketFd,data,size,0);
        if(readSize>0)
            return readSize;
        else
        {
            if(readSize==0)
            {
                errorString="connection closed by peer";
                return -1;
            }
            else
            {
                if(errno==EINTR)
                {
                    //retry, a signal interrupted the syscall
                }
                else
                {
                    if(errno==EAGAIN || errno==EWOULDBLOCK)
                        return 0;//nothing pending, not an error
                    else
                    {
                        errorString=std::string("recv() failed: ")+strerror(errno);
                        return -1;
                    }
                }
            }
        }
    }
}

ssize_t CliSocket::writeSome(const char * const data,const size_t &size)
{
    if(socketFd<0)
        return -1;
    if(size==0)
        return 0;
    while(true)
    {
        const ssize_t writtenSize=::send(socketFd,data,size,MSG_NOSIGNAL);
        if(writtenSize>=0)
            return writtenSize;
        else
        {
            if(errno==EINTR)
            {
                //retry, a signal interrupted the syscall
            }
            else
            {
                if(errno==EAGAIN || errno==EWOULDBLOCK)
                    return 0;//send buffer full: the peer stopped draining
                else
                {
                    errorString=std::string("send() failed: ")+strerror(errno);
                    return -1;
                }
            }
        }
    }
}

void CliSocket::closeSocket()
{
    if(socketFd>=0)
    {
        if(::close(socketFd)<0)
            std::cerr << "CliSocket: close() failed: " << strerror(errno) << std::endl;
        socketFd=-1;
    }
    connecting=false;
}

const std::string &CliSocket::lastError() const
{
    return errorString;
}
