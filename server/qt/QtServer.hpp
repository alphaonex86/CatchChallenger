#ifndef CATCHCHALLENGER_QTSERVER_H
#define CATCHCHALLENGER_QTSERVER_H

#ifndef NOTHREADS
#include <QThread>
#else
#include <QObject>
#endif
#if defined(CATCHCHALLENGER_SOLO) && ! defined(CATCHCHALLENGER_NO_TCPSOCKET) && defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_MULTI)
#include <QTcpServer>
#include <QUdpSocket>
#include <QTimer>
#endif
#include "../base/BaseServer/BaseServer.hpp"
#include "../../general/base/lib.h"
#include "QtServerStructures.hpp"
#include "QtClient.hpp"

#ifndef NOTHREADS
class DLL_PUBLIC QtServer : public QThread, public CatchChallenger::BaseServer
#else
class DLL_PUBLIC QtServer : public QObject, public CatchChallenger::BaseServer
#endif
{
    Q_OBJECT
public:
    QtServer();
    static QtServerPrivateVariables *qtServerPrivateVariables;
    virtual ~QtServer();
    void preload_the_city_capture();
    void removeOneClient();
    void newConnection();
    void connect_the_last_client(CatchChallenger::Client * client, QIODevice *socket);
    void load_next_city_capture();
    void send_insert_move_remove();
    void positionSync();
    void ddosTimer();
    virtual void start();
    bool isListen();
    bool isStopped();
    void stop();
    bool check_if_now_stopped();//return true if can be stopped
    void stop_internal_server();
    virtual void preload_finish() override;
    void quitForCriticalDatabaseQueryFailed() override;
    void loadAndFixSettings() override;
    // Surface datapack-load failures via the error() signal instead of
    // aborting (the BaseServer default). Lets the GUI pop a
    // QMessageBox::warning and stay alive so the operator can fix the
    // datapack and retry.
    void cc_datapack_fail(const std::string &msg) override;
#ifdef CATCHCHALLENGER_CLASS_QT
    // ── Live-stats probes (GUI-only) ────────────────────────────────
    // The dashboard (server/qt/gui/MainWindow) polls these every second
    // to render real numbers instead of synthetic data. They're gated
    // on CATCHCHALLENGER_CLASS_QT so the headless epoll/io_uring path
    // pays nothing — no atomics, no per-byte counter increments.
    //
    // currentClientCount(): live size of client_list (in-process count
    //     of connected QtClient instances, both real-TCP and QFakeSocket).
    // pingMs/Min/Avg/Max(): per-client last-known ping in ms; zeroes
    //     when no clients are connected. Pulled from each Client's
    //     ProtocolParsingInputOutput latency snapshot.
    size_t currentClientCount() const;
    void clientPingStats(uint32_t &minOut, uint32_t &avgOut, uint32_t &maxOut) const;
#endif

    void setEventTimer(const uint8_t &event,const uint8_t &value,const unsigned int &time,const unsigned int &start) override;
    void preload_the_visibility_algorithm() override;
    void unload_the_visibility_algorithm() override;
    void unload_the_events() override;
    #if defined(CATCHCHALLENGER_SOLO) && ! defined(CATCHCHALLENGER_NO_TCPSOCKET) && defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_MULTI)
    void openToLan(QString name,bool allowInternet=true);//for now internet filter not implemented
    bool openIsOpenToLan();
    #endif
signals:
    #if defined(CATCHCHALLENGER_SOLO) && ! defined(CATCHCHALLENGER_NO_TCPSOCKET) && defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_MULTI)
    void emitLanPort(uint16_t port);
    #endif
    void try_initAll() const;
    void try_stop_server() const;
    void need_be_started() const;
    //stat
    void is_started(const bool &) const;
    void error(const std::string &error) const;
    void stop_internal_server_signal();

    void haveQuitForCriticalDatabaseQueryFailed();
private:
    std::unordered_set<CatchChallenger::Client *> client_list;
    #if defined(CATCHCHALLENGER_SOLO) && ! defined(CATCHCHALLENGER_NO_TCPSOCKET) && defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_MULTI)
    QTcpServer server;
    QUdpSocket broadcastLan;
    QTimer broadcastLanTimer;
    QByteArray dataToSend;
    void sendBroadcastServer();
    #endif
private:
    void stop_internal_server_slot();
};

#endif // CATCHCHALLENGER_QTSERVER_H
