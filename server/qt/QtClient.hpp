#ifndef CATCHCHALLENGER_QTCLIENT_H
#define CATCHCHALLENGER_QTCLIENT_H

#include <QIODevice>

#ifdef CATCHCHALLENGER_CLASS_QT
#include <atomic>
#include <cstdint>
#endif

namespace CatchChallenger {

#ifdef CATCHCHALLENGER_CLASS_QT
// Process-wide network-throughput counters consumed by the Qt server-gui
// dashboard ("Network traffic" chart). Defined in QtClient.cpp; bumped by
// every QtClient::read()/write() call so both real-TCP and QFakeSocket
// clients are captured. Headless builds never see these symbols.
extern std::atomic<uint64_t> qtNetStats_bytesReceived;
extern std::atomic<uint64_t> qtNetStats_bytesSent;
#endif

class QtClient
{
public:
    QtClient(QIODevice *qtSocket);
    ssize_t read(char * data, const size_t &size);
    ssize_t write(const char * const data, const size_t &size);
    bool disconnectClientTimer();
    bool disconnectClient();
    void closeSocket();
    bool isValid();
public:
    QIODevice *socket;
};
}

#endif // CATCHCHALLENGER_QTSERVER_H
