#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLocalSocket>
#include <QList>
#include <QCloseEvent>

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
    QTimer timer;
    QLocalSocket socket;
    bool inClosing;
    QList<double> timeList,timeListForUser,timeListForTimer,timeListForDatabase;
    bool waitTheReturn;
public slots:
    void closeEvent(QCloseEvent *event);
    void updateUserTime();
    void readyRead();
    void updateSocketState(QLocalSocket::LocalSocketState socketState);
};

#endif // MAINWINDOW_H
