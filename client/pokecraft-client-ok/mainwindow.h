#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QRegExp>

#include "Pokecraft_client.h"

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
	void on_lineEditLogin_editingFinished();
	void on_lineEditPass_editingFinished();
	void on_checkBoxRememberPassword_stateChanged(int arg1);
	void on_lineEditLogin_returnPressed();
	void on_lineEditPass_returnPressed();
	void on_pushButtonTryLogin_clicked();
	void stateChanged(QAbstractSocket::SocketState socketState);
	void error(QAbstractSocket::SocketError socketError);
	void haveNewError();
	void disconnected(QString reason);
	void notLogged(QString reason);
	void logged();
	void protocol_is_good();
	void have_current_player_info();
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

private:
	Ui::MainWindow *ui;
	Pokecraft_client client;
	void resetAll();
	QList<quint32> chat_list_player_id;
	QList<quint8> chat_list_type;
	QList<QString> chat_list_text;
	QString toHtmlEntities(QString text);
	QSettings settings;
	QString lastMessageSend;
	QTimer stopFlood;
	int numberForFlood;
	QString toSmilies(QString text);
};

#endif // MAINWINDOW_H
