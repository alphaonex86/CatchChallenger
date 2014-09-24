#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTime>
#include <QLocalSocket>
#include <QList>
#include <QCloseEvent>
#include <QSettings>
#include <QProcess>

#include "../bot/MultipleBotConnectionImplFoprGui.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private:
    Ui::MainWindow *ui;
    //QTimer timer;
    QLocalSocket socket;
    bool inClosing;
    QList<double> timeList,timeListForUser,timeListForTimer,timeListForDatabase;
    bool waitTheReturn;
    QSettings settings;
    QProcess process;
    bool serverStarted;
    MultipleBotConnectionImplFoprGui multipleBotConnection;
    bool dropTime;
    bool dropStat;
    QString returnType;
    QTimer stopBenchmarkTimer;
    int stopBenchmarkCount=0;
    QString convertUsToString(quint64 us);
    double systemItem;
    QTime time;
    long systemClockTick;
public slots:
    void closeEvent(QCloseEvent *event);
    void updateUserTime();
    void readyRead();
    void updateSocketState(QLocalSocket::LocalSocketState socketState);
    void stateChanged(QProcess::ProcessState newState);
    void processError(QProcess::ProcessError error);
private slots:
    void on_server_editingFinished();
    void on_serverBrowse_clicked();
    void on_start_clicked();
    void readyReadStandardOutput();
    void readyReadStandardError();
    void startServer();
    void all_player_on_map();
    void on_benchmarkIdle_clicked();
    void on_benchmarkMove_clicked();
    void stopBenchmark();
    void on_benchmarkChat_clicked();
    void resetBotConnection();
    void startSystemMeasure();
    void stopSystemMeasure();
    double getSystemTime();
};

#endif // MAINWINDOW_H
