#include "QtClient.hpp"

#ifdef CATCHCHALLENGER_CLASS_QT
#include <atomic>
#include <cstdint>
#endif
// Mirror this layer's qtNetStats_* atomics into the unified
// general/base/CCGuiStats counters when CATCHCHALLENGER_GUI_STATS is
// on (catchchallenger-server-gui only).  No-op in non-GUI binaries.
#include "../../general/base/CCGuiStats.hpp"

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
    // Issue the read first, then bump the counter with the ACTUAL bytes
    // returned (partial reads + EOF return less than the requested
    // buffer size).  The dashboard wants real bytes-on-the-wire, not an
    // upper bound.  Errors return -1; skip the bump in that case so a
    // disconnect-with-pending-read doesn't poison the counter.
    const ssize_t got = socket->read(data, size);
    #ifdef CATCHCHALLENGER_GUI_STATS
    if (got > 0)
        CatchChallenger::gui_stats::net_bytes_received.fetch_add(
            static_cast<uint64_t>(got), std::memory_order_relaxed);
    #endif
    #ifdef CATCHCHALLENGER_CLASS_QT
    if (got > 0)
        qtNetStats_bytesReceived.fetch_add(
            static_cast<uint64_t>(got), std::memory_order_relaxed);
    #endif
    return got;
}

ssize_t QtClient::write(const char * const data, const size_t &size)
{
    //do some basic check on low level protocol (message split, ...)
    /*if(ProtocolParsingInputOutput::write(data,size)<0)
        return -1;*/
    if(socket==nullptr)
        abort();
    // Same policy as read(): write, then count real bytes-handed-off.
    // QIODevice::write() can write less than requested under back-
    // pressure; counting the return value matches what the kernel /
    // QSslSocket actually accepted.
    const ssize_t put = socket->write(data, size);
    #ifdef CATCHCHALLENGER_GUI_STATS
    if (put > 0)
        CatchChallenger::gui_stats::net_bytes_sent.fetch_add(
            static_cast<uint64_t>(put), std::memory_order_relaxed);
    #endif
    #ifdef CATCHCHALLENGER_CLASS_QT
    if (put > 0)
        qtNetStats_bytesSent.fetch_add(
            static_cast<uint64_t>(put), std::memory_order_relaxed);
    #endif
    return put;
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

