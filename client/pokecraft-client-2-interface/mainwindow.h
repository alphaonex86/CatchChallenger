#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QRegExp>
#include <QAbstractSocket>

#include "../../general/base/ChatParsing.h"
#include "pokecraft-clients/graphicsviewkeyinput.h"
#include "pokecraft-clients/craft-clients.h"
#include "../../general/base/GeneralStructures.h"
#include "../base/Api_client_real.h"

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
	void newError(QString error,QString detailedError);
	void error(QString error);
	void message(QString message);
	void disconnected(QString reason);
	void notLogged(QString reason);
	void logged();
	void protocol_is_good();
	void have_current_player_info();
	void haveTheDatapack();
	void on_lineEdit_chat_text_returnPressed();
	void new_chat_text(quint32 player_id,quint8 chat_type,QString text);
	void on_pushButton_interface_bag_pressed();
	void on_pushButton_interface_bag_released();
	void on_pushButton_interface_monster_list_pressed();
	void on_pushButton_interface_monster_list_released();
	void on_pushButton_interface_trainer_pressed();
	void on_pushButton_interface_trainer_released();
	void on_toolButton_interface_map_pressed();
	void on_toolButton_interface_map_released();
	void number_of_player(quint16 number,quint16 max);
	void on_comboBox_chat_type_currentIndexChanged(int index);
	void update_chat();
	void removeNumberForFlood();
	void on_toolButton_interface_quit_clicked();
	void on_toolButton_interface_quit_pressed();
	void on_toolButton_interface_quit_released();
	void on_toolButton_quit_interface_clicked();
	void on_toolButton_quit_interface_pressed();
	void on_toolButton_quit_interface_released();
	void on_pushButton_interface_trainer_clicked();
private:
	Ui::MainWindow *ui;
	Pokecraft::Api_client_real *client;
	void resetAll();
	QList<quint32> chat_list_player_id;
	QList<Pokecraft::Chat_type> chat_list_type;
	QList<QString> chat_list_text;
	QString toHtmlEntities(QString text);
	QSettings settings;
	QString lastMessageSend;
	QTimer stopFlood;
	int numberForFlood;
	bool haveShowDisconnectionReason;
	QString toSmilies(QString text);
        graphicsviewkeyinput *graphicsview;
        craftClients *subclient;
	QStringList server_list;
	QTcpSocket socket;
};

#endif // MAINWINDOW_H
