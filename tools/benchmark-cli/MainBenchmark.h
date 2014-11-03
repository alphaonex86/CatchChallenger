#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTimer>
#include <QTime>
#include <QLocalSocket>
#include <QList>
#include <QSettings>
#include <QProcess>

#include "../bot/MultipleBotConnectionImplFoprGui.h"

namespace Ui {
class MainWindow;
}

class MainBenchmark : public QObject
{
    Q_OBJECT

public:
    explicit MainBenchmark();
    ~MainBenchmark();
private:
    //QTimer timer;
    bool inClosing;
    QSettings settings;
    QProcess process;
    bool serverStarted;
    MultipleBotConnectionImplFoprGui multipleBotConnection;
    QTimer stopBenchmarkTimer;
    double systemItem;
    QTime time;

    QTimer startAll;
    QString server;
    quint32 multipleConnexion;
    quint32 connectBySeconds;
    quint32 maxDiffConnectedSelected;
    quint8 benchmarkStep;
    QLocalSocket socket;
public slots:
    void updateUserTime();
    void processError(QProcess::ProcessError error);
    void processStateChanged(const QProcess::ProcessState &stateChanged);
private slots:
    void readyReadStandardOutput();
    void readyReadStandardError();
    void startServerConnexion();
    void all_player_on_map();
    void stopBenchmark();
    void resetBotConnection();
    void startSystemMeasure();
    void stopSystemMeasure();
    double getSystemTime();

    void benchmarkIdle();
    void benchmarkMove();
    void benchmarkChat();

    void init();
    void readyRead();
};

#endif // MAINWINDOW_H
