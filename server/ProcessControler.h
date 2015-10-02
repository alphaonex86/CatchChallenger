#ifndef CATCHCHALLENGER_PROCESSCONTROLER_H
#define CATCHCHALLENGER_PROCESSCONTROLER_H

#include <QCoreApplication>

#include "NormalServer.h"
#include "../general/base/DebugClass.h"
#include "../general/base/GeneralStructures.h"

class ProcessControler : public QObject
{
    Q_OBJECT
public:
    explicit ProcessControler();
    ~ProcessControler();
protected:
    void changeEvent(QEvent *e);
private:
    CatchChallenger::NormalServer server;
    bool need_be_restarted;
    bool need_be_closed;
    std::string sizeToString(double size);
    std::string adaptString(float size);
    std::unordered_settings *settings;
    void send_settings();
    void haveQuitForCriticalDatabaseQueryFailed();
private slots:
    void server_is_started(bool is_started);
    void server_need_be_stopped();
    void server_need_be_restarted();
    void error(const std::string &error);
signals:
    void record_latency();
};

#endif // MAINWINDOW_H
