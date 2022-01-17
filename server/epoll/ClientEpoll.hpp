#ifndef CATCHCHALLENGER_SERVEREpoll_H
#define CATCHCHALLENGER_SERVEREpoll_H

#include <QObject>
#include "BaseClassSwitch.hpp"

class ClientEpoll : public BaseClassSwitch, public Client
{
public:
    ClientEpoll();
    ssize_t read(char * data, const size_t &size);
    ssize_t write(const char * const data, const size_t &size);
    bool disconnectClientTimer();
    bool disconnectClient();
    BaseClassSwitch::EpollObjectType getType() const;
};

#endif // CATCHCHALLENGER_QTSERVER_H
