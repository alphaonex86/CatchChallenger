#ifndef CLIENTWITHSOCKET_H
#define CLIENTWITHSOCKET_H

#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QObject>
#include <QTimer>
#include <QAbstractSocket>
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
        public ConnectedSocket,
        #endif
        public Client
{
#ifndef EPOLLCATCHCHALLENGERSERVER
    Q_OBJECT
#endif
public:
    ClientWithSocket();
    ssize_t read(char * data, const size_t &size);
    ssize_t write(const char * const data, const size_t &size);
    bool disconnectClient();
    void closeSocket();
};
}

#endif // CLIENTWITHSOCKET_H
