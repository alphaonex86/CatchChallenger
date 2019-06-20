#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWebSockets/QWebSocket>

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
    void onConnected();
    void onClosed();
    void onBinaryMessageReceived(QByteArray message);
    void on_tryConnect_clicked();

private:
    Ui::MainWindow *ui;
    QWebSocket m_webSocket;
};

#endif // MAINWINDOW_H
