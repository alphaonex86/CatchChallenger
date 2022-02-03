#ifndef P2PCLIENT_H
#define P2PCLIENT_H

#include "../../server/epoll/BaseClassSwitch.h"
#include <string>
#include <vector>
#include <netdb.h>

class P2PClient : public BaseClassSwitch
{
public:
    P2PClient(const int &infd);
    ~P2PClient();
    void parseIncommingData();
    BaseClassSwitch::EpollObjectType getType() const;

    static std::vector<std::pair<std::string,uint16_t> > hostToConnect;
    void close();
    ssize_t read(char *bufferClearToOutput,const size_t &bufferSizeClearToOutput);
    ssize_t write(const char *bufferClearToOutput, const size_t &bufferSizeClearToOutput);
    bool isValid() const;
    long int bytesAvailable() const;
    int getinfd();
private:
    int infd;

    void tryGetData();

    char header[5];//if(header[0]==4) then have the message size
    std::string data;
    static char readBuffer[4096];
    bool sendHello;
};

#endif // P2PCLIENT_H
