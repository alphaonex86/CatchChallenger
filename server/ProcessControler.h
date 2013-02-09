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
    QString sizeToString(double size);
    QString adaptString(float size);
    QSettings *settings;
    void send_settings();
private slots:
    void server_is_started(bool is_started);
    void server_need_be_stopped();
    void server_need_be_restarted();
    void benchmark_result(int latency,double TX_speed,double RX_speed,double TX_size,double RX_size,double second);
    void error(const QString &error);
signals:
    void record_latency();
};

#endif // MAINWINDOW_H
