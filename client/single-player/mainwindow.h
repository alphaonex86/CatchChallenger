#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QRegExp>
#include <QAbstractSocket>
#include <QSettings>
#include <QTimer>
#include <QSpacerItem>

#include "../../general/base/QFakeSocket.h"
#include "../../general/base/ChatParsing.h"
#include "../../general/base/GeneralStructures.h"
#include "../base/Api_client_real.h"
#include "../base/interface/MapController.h"
#include "../base/interface/BaseWindow.h"
#include "SaveGameLabel.h"
#include "InternalServer.h"

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
	void stateChanged(QAbstractSocket::SocketState socketState);
	void error(QAbstractSocket::SocketError socketError);
	void haveNewError();
	void message(QString message);
    void disconnected(QString reason);
    void protocol_is_good();
    void try_stop_server();
    void on_SaveGame_New_clicked();
    void savegameLabelClicked();
    void savegameLabelUpdate();
    void on_SaveGame_Delete_clicked();
    void on_SaveGame_Rename_clicked();
    void on_SaveGame_Copy_clicked();
    void on_SaveGame_Play_clicked();
    void needQuit();
private:
	Ui::MainWindow *ui;
    Pokecraft::Api_client_virtual *client;
	void resetAll();
    bool rmpath(const QDir &dir);
    void updateSavegameList();
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
    Pokecraft::QFakeSocket socket;
    Pokecraft::BaseWindow *baseWindow;
    QList<SaveGameLabel *> savegame;
    QHash<SaveGameLabel *,QString> savegamePath;
    QHash<SaveGameLabel *,bool> savegameWithMetaData;
    SaveGameLabel * selectedSavegame;
    QSpacerItem *spacer;
    Pokecraft::InternalServer * internalServer;
};

#endif // MAINWINDOW_H
