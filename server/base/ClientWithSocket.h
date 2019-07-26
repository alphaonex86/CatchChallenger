#ifndef CLIENTWITHSOCKET_H
#define CLIENTWITHSOCKET_H

#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QObject>
#include <QTimer>
#include <QAbstractSocket>
#include "../../client/qt/ConnectedSocket.h"
#else
#include "../epoll/BaseClassSwitch.h"
#include "../epoll/EpollClient.h"
#endif

#include "Client.h"

namespace CatchChallenger {
class ClientWithSocket :
        #ifdef EPOLLCATCHCHALLENGERSERVER
        public EpollClient,
        #else
        public QObject,
        #endif
        public Client
{
public:
    ClientWithSocket();
    ssize_t read(char * data, const size_t &size) override;
    ssize_t write(const char * const data, const size_t &size) override;
    bool disconnectClient() override;
    void closeSocket() override;

    #ifndef EPOLLCATCHCHALLENGERSERVER
    void parseIncommingData() override;
    QIODevice *qtSocket;
    #endif
};
}

#endif // CLIENTWITHSOCKET_H
