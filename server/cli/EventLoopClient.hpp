#ifndef EVENT_LOOP_CLIENT_H
#define EVENT_LOOP_CLIENT_H

#include "BaseClassSwitch.hpp"
#include <sys/types.h>

#define BUFFER_MAX_SIZE 4096

namespace CatchChallenger {
class EventLoopClient : public BaseClassSwitch
{
public:
    EventLoopClient(const int &infd);
    ~EventLoopClient();
    void reopen(const int &infd);
    void close();
    ssize_t read(char *buffer,const size_t &bufferSize);
    ssize_t write(const char *buffer,const size_t &bufferSize);
    EventLoopObjectType getType() const;
    bool isValid() const;
    long int bytesAvailable() const;

    void closeSocket();
    bool socketIsOpen();
    bool socketIsClosed();
protected:
    int infd;
};
}

#endif // EVENT_LOOP_CLIENT_H
