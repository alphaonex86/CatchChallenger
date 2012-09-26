#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QRegExp>
#include <QAbstractSocket>
#include <QSettings>
#include <QTimer>

#include "../../general/base/ChatParsing.h"
#include "../../general/base/GeneralStructures.h"
#include "../base/Api_client_real.h"
#include "../base/interface/MapController.h"
#include "../base/interface/BaseWindow.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
protected:
	void changeEvent(QEvent *e);
private slots:
	void on_pushButtonTryLogin_pressed();
	void on_pushButtonTryLogin_released();
	void on_lineEditLogin_returnPressed();
	void on_lineEditPass_returnPressed();
	void on_pushButtonTryLogin_clicked();
	void stateChanged(QAbstractSocket::SocketState socketState);
	void error(QAbstractSocket::SocketError socketError);
	void haveNewError();
	void message(QString message);
    void disconnected(QString reason);
    void protocol_is_good();
private:
	Ui::MainWindow *ui;
	Pokecraft::Api_client_real *client;
	void resetAll();
	QStringList chat_list_player_pseudo;
	QList<Pokecraft::Player_type> chat_list_player_type;
	QList<Pokecraft::Chat_type> chat_list_type;
    QList<QString> chat_list_text;
	QSettings settings;
	QString lastMessageSend;
	QTimer stopFlood;
	int numberForFlood;
    bool haveShowDisconnectionReason;
	QStringList server_list;
    QTcpSocket socket;
    Pokecraft::BaseWindow *baseWindow;
};

#endif // MAINWINDOW_H
