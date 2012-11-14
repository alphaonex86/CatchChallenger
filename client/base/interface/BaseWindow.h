#include <QWidget>
#include <QMessageBox>
#include <QRegExp>
#include <QAbstractSocket>
#include <QSettings>
#include <QTimer>
#include <QListWidgetItem>

#include "../../general/base/ChatParsing.h"
#include "../../general/base/GeneralStructures.h"
#include "../base/Api_client_real.h"
#include "../base/Api_protocol.h"
#include "MapController.h"
#include "DatapackClientLoader.h"

#ifndef POKECRAFT_BASEWINDOW_H
#define POKECRAFT_BASEWINDOW_H

namespace Ui {
    class BaseWindowUI;
}

namespace Pokecraft {
class BaseWindow : public QWidget
{
    Q_OBJECT
public:
    explicit BaseWindow(Pokecraft::Api_protocol *client);
    ~BaseWindow();
    void setMultiPlayer(bool multiplayer);
    void resetAll();
    void serverIsLoading();
    void serverIsReady();
    QString lastLocation() const;
protected:
    void changeEvent(QEvent *e);
public slots:
    void stateChanged(QAbstractSocket::SocketState socketState);
private slots:
    void message(QString message);
    void disconnected(QString reason);
    void notLogged(QString reason);
    void on_lineEdit_chat_text_returnPressed();
    void protocol_is_good();
    void newError(QString error,QString detailedError);
    void error(QString error);

    //chat
    void new_chat_text(Pokecraft::Chat_type chat_type,QString text,QString pseudo,Pokecraft::Player_type type);
    void new_system_text(Pokecraft::Chat_type chat_type,QString text);

    //autoconnect
    void number_of_player(quint16 number,quint16 max);
    void on_comboBox_chat_type_currentIndexChanged(int index);
    void update_chat();
    void removeNumberForFlood();
    void on_toolButton_interface_quit_clicked();
    void on_toolButton_quit_interface_clicked();
    void on_pushButton_interface_trainer_clicked();
    void on_lineEdit_chat_text_lostFocus();

    //player UI
    void on_pushButton_interface_bag_clicked();
    void on_toolButton_quit_interface_2_clicked();
    void on_inventory_itemSelectionChanged();
    //player
    void logged();
    void updatePlayerImage();
    void have_current_player_info();
    void have_inventory(const QHash<quint32,quint32> &items);
    void load_inventory();

    //datapack
    void haveTheDatapack();
protected slots:
    //datapack
    void datapackParsed();
    //UI
    void updateConnectingStatus();
private:
    Ui::BaseWindowUI *ui;
    Pokecraft::Api_protocol *client;
    QStringList chat_list_player_pseudo;
    QList<Pokecraft::Player_type> chat_list_player_type;
    QList<Pokecraft::Chat_type> chat_list_type;
    QList<QString> chat_list_text;
    QString toHtmlEntities(QString text);
    QSettings settings;
    QString lastMessageSend;
    QTimer stopFlood;
    int numberForFlood;
    bool haveShowDisconnectionReason;
    QString toSmilies(QString text);
    QStringList server_list;
    MapController *mapController;
    QAbstractSocket::SocketState socketState;
    QStringList skinFolderList;
    bool haveDatapack,havePlayerInformations,haveInventory,datapackIsParsed;
    DatapackClientLoader datapackLoader;

    //player info
    QHash<quint32,quint32> items;
    QHash<QListWidgetItem *,quint32> items_graphical;
signals:
    //datapack
    void parseDatapack(const QString &datapackPath);
};
}

#endif
