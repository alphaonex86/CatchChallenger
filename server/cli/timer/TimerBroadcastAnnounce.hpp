#ifndef TIMERBROADCASTANNOUNCE_H
#define TIMERBROADCASTANNOUNCE_H

#include "../EventLoopTimer.hpp"
#include <string>
#include <vector>
#include <cstdint>

// TimerBroadcastAnnounce — periodic LAN "here I am" announce.
//
// Sends the SAME UDP broadcast datagram the desktop "open to LAN" server sends
// (server/qt/QtServer.cpp::sendBroadcastServer), so the existing client LAN
// watcher (client/libqtcatchchallenger/LanBroadcastWatcher.cpp, UDP port 42490)
// auto-discovers this server and lists it under the configured name.
//
// The desktop server builds the payload with QDataStream(Qt_4_4):
//     stream << QString(name);          // quint32 byteCount + UTF-16 (BigEndian)
//     stream << quint16(serverPort);    // BigEndian
// We reproduce those bytes by hand so this headless / ESP32 server needs no Qt.
//
// Off unless a non-empty name is configured (GameServerSettings::broadcastName).
// This is what announces a wifi-connected ESP32 server to LAN clients.
class TimerBroadcastAnnounce : public EventLoopTimer
{
public:
    TimerBroadcastAnnounce(const std::string &name, uint16_t serverPort);
    ~TimerBroadcastAnnounce();
    void exec();
    bool socketReady() const;
private:
    int fd;                      // UDP socket with SO_BROADCAST; -1 when unavailable
    uint16_t destPort;           // client watcher port (42490)
    std::vector<char> datagram;  // precomputed Qt_4_4 payload (name + port)
};

#endif // TIMERBROADCASTANNOUNCE_H
