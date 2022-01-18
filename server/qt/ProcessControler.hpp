#ifndef CATCHCHALLENGER_PROCESSCONTROLER_H
#define CATCHCHALLENGER_PROCESSCONTROLER_H

#include <QCoreApplication>

#include "NormalServer.hpp"
#include "../general/base/GeneralStructures.hpp"
#include "../general/base/FacilityLibGeneral.hpp"
#include "../general/base/FacilityLib.hpp"
#include "base/TinyXMLSettings.hpp"
#include "base/GlobalServerData.hpp"

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
    TinyXMLSettings *settings;
    void send_settings();
    void haveQuitForCriticalDatabaseQueryFailed();
    void generateTokenStatClient(TinyXMLSettings &settings,char * const data);
private slots:
    void server_is_started(bool is_started);
    void server_need_be_stopped();
    void server_need_be_restarted();
    void error(const std::string &error);
signals:
    void record_latency();
};

#endif // MAINWINDOW_H
