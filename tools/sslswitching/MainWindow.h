#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSslSocket>
#include "Server.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private slots:
    void on_test_clicked();
private:
    void test_socket();
    void newConnection();
    void newServerSocketData();
    void clientReadyRead();
    void clientEncrypted();
private:
    Ui::MainWindow *ui;
    Server *server;
    QSslSocket *client;
    bool modeNegociated;
};

#endif // MAINWINDOW_H
