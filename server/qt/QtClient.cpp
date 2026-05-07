#include "QtClient.hpp"

#ifdef CATCHCHALLENGER_CLASS_QT
#include <atomic>
#include <cstdint>
#endif

using namespace CatchChallenger;

#ifdef CATCHCHALLENGER_CLASS_QT
// Live network-throughput probes for the GUI dashboard. Process-wide
// atomic counters bumped on every QtClient::read/write — covers both
// real TCP clients and solo clients via QFakeSocket. Read by the
// MainWindow once per second to compute a rolling B/s rate for the
// "Network traffic" chart. Definitions tied to the CATCHCHALLENGER_CLASS_QT
// guard so the headless build never sees them.
namespace CatchChallenger {
std::atomic<uint64_t> qtNetStats_bytesReceived{0};
std::atomic<uint64_t> qtNetStats_bytesSent{0};
}
#endif

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
    // Bump the GUI dashboard's RX counter immediately before the actual
    // QTcpSocket read. We charge the requested buffer size — partial
    // reads get over-counted slightly, but it keeps the increment on
    // the same line as the socket call so the boundary is unambiguous.
    #ifdef CATCHCHALLENGER_CLASS_QT
    qtNetStats_bytesReceived.fetch_add(static_cast<uint64_t>(size), std::memory_order_relaxed);
    #endif
    return socket->read(data,size);
}

ssize_t QtClient::write(const char * const data, const size_t &size)
{
    //do some basic check on low level protocol (message split, ...)
    /*if(ProtocolParsingInputOutput::write(data,size)<0)
        return -1;*/
    if(socket==nullptr)
        abort();
    // Bump the GUI dashboard's TX counter immediately before the actual
    // QTcpSocket write. `size` is exactly what we hand the socket — the
    // counter reflects bytes-handed-off, not bytes-the-kernel-accepted.
    #ifdef CATCHCHALLENGER_CLASS_QT
    qtNetStats_bytesSent.fetch_add(static_cast<uint64_t>(size), std::memory_order_relaxed);
    #endif
    return socket->write(data,size);
}

bool QtClient::disconnectClient()
{
    if(socket==nullptr)
        abort();
    socket->close();
    //Client::disconnectClient();
    return true;

}

void QtClient::closeSocket()
{
    disconnectClient();
}

bool QtClient::isValid()
{
    return socket->isOpen();
}

