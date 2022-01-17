#include "QtClient.hpp"

using namespace CatchChallenger;

QtClient::QtClient(QIODevice *socket) :
    socket(socket)
{
}

bool QtClient::disconnectClientTimer()
{
    return true;
}

ssize_t QtClient::read(char * data, const size_t &size)
{
    if(socket==nullptr)
        abort();
    return socket->read(data,size);//count RXSize
}

ssize_t QtClient::write(const char * const data, const size_t &size)
{
    //do some basic check on low level protocol (message split, ...)
    if(ProtocolParsingInputOutput::write(data,size)<0)
        return -1;
    if(socket==nullptr)
        abort();
    return socket->write(data,size);
}

bool QtClient::disconnectClient()
{
    if(socket==nullptr)
        abort();
    socket->close();
    Client::disconnectClient();
    return true;

}

void QtClient::closeSocket()
{
    disconnectClient();
}

