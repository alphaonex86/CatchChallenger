#ifndef EVENT_LOOP_CLIENT_TO_SERVER_H
#define EVENT_LOOP_CLIENT_TO_SERVER_H

#include "BaseClassSwitch.hpp"
#include <sys/types.h>

#define BUFFER_MAX_SIZE 4096

namespace CatchChallenger {
class EventLoopClientToServer : public BaseClassSwitch
{
public:
    EventLoopClientToServer(const int &infd);
    ~EventLoopClientToServer();
    void close();
    ssize_t read(char *buffer,const size_t &bufferSize);
    ssize_t write(const char *buffer,const size_t &bufferSize);
    void flush();
    EventLoopObjectType getType() const;
    bool isValid() const;
    long int bytesAvailable() const;
private:
    int infd;
};
}

#endif // EVENT_LOOP_CLIENT_TO_SERVER_H
