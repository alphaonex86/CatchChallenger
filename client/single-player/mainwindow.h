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
    void disconnected(QString);
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
    void is_started(bool started);
private:
    Ui::MainWindow *ui;
    Pokecraft::Api_client_virtual *client;
    void resetAll();
    bool rmpath(const QDir &dir);
    void updateSavegameList();
    QSettings settings;
    QString lastMessageSend;
    QStringList server_list;
    Pokecraft::ConnectedSocket *socket;
    Pokecraft::BaseWindow *baseWindow;
    QList<SaveGameLabel *> savegame;
    QHash<SaveGameLabel *,QString> savegamePath;
    QHash<SaveGameLabel *,bool> savegameWithMetaData;
    SaveGameLabel * selectedSavegame;
    QSpacerItem *spacer;
    Pokecraft::InternalServer * internalServer;
    QString pass;
};

#endif // MAINWINDOW_H
