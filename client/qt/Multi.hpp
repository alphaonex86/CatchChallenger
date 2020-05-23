#ifndef MULTI_H
#define MULTI_H

#include <QWidget>
#include <QString>
#include <QHash>
#include <QSet>
#include <vector>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVBoxLayout>
#include "CCWidget.hpp"

class ListEntryEnvolued;
class AddOrEditServer;
class Login;

namespace Ui {
class Multi;
}

class SelectedServer
{
public:
    QString unique_code;
    bool isCustom;
};

class Multi : public QWidget
{
    Q_OBJECT

public:
    class ConnexionInfo
    {
    public:
        QString unique_code;
        QString name;
        bool isCustom;

        //hightest priority
        QString host;
        uint16_t port;
        //lower priority
        QString ws;

        uint32_t connexionCounter;
        uint32_t lastConnexion;

        QString register_page;
        QString lost_passwd_page;
        QString site_page;

        QString proxyHost;
        uint16_t proxyPort;

        bool operator<(const ConnexionInfo &connexionInfo) const;
    };

    explicit Multi(QWidget *parent = nullptr);
    ~Multi();
    void displayServerList();
    void serverListEntryEnvoluedClicked();
    void server_add_clicked();
    void server_add_finished();
    void server_edit_clicked();
    void server_edit_finished();
    void server_select_clicked();
    void server_select_finished();
    void server_remove_clicked();
    void saveConnexionInfoList();
    void serverListEntryEnvoluedDoubleClicked();
    std::vector<ConnexionInfo> loadXmlConnexionInfoList();
    std::vector<ConnexionInfo> loadXmlConnexionInfoList(const QByteArray &xmlContent);
    std::vector<ConnexionInfo> loadXmlConnexionInfoList(const QString &file);
    std::vector<ConnexionInfo> loadConfigConnexionInfoList();
    void httpFinished();
    void downloadFile();
private:
    Ui::Multi *ui;

    std::vector<ConnexionInfo> temp_customConnexionInfoList,temp_xmlConnexionInfoList,mergedConnexionInfoList;
    QHash<ListEntryEnvolued *,ConnexionInfo *> serverConnexion;
    SelectedServer selectedServer;//no selected if unique_code empty
    AddOrEditServer *addServer;
    Login *login;

    QNetworkAccessManager qnam;
    QNetworkReply *reply;
signals:
    void backMain();
    void setAbove(QWidget *widget);//first plan popup
    void connectToServer(ConnexionInfo connexionInfo,QString login,QString pass);
};

#endif // MULTI_H
