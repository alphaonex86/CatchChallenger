#ifndef CATCHCHALLENGER_QTCLIENT_H
#define CATCHCHALLENGER_QTCLIENT_H

#include <QIODevice>
#include "../base/Client.hpp"

namespace CatchChallenger {
class QtClient : public Client
{
public:
    QtClient(QIODevice *socket);
    ssize_t read(char * data, const size_t &size);
    ssize_t write(const char * const data, const size_t &size);
    bool disconnectClientTimer();
    bool disconnectClient();
public:
    QIODevice *socket;
};
}

#endif // CATCHCHALLENGER_QTSERVER_H
