#ifndef MULTI_H
#define MULTI_H

#include <QWidget>
#include <QString>
#include <QHash>
#include <QSet>
#include <QLabel>
#include <vector>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "CCWidget.hpp"
#include "ScreenInput.hpp"
#include "CustomButton.hpp"

class ListEntryEnvolued;
class AddOrEditServer;
class Login;

class SelectedServer
{
public:
    QString unique_code;
    bool isCustom;
};

class Multi : public QObject, public ScreenInput
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

    explicit Multi();
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
    void newLanguage();
    void on_server_refresh_clicked();
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
private:
    std::vector<ConnexionInfo> temp_customConnexionInfoList,temp_xmlConnexionInfoList,mergedConnexionInfoList;
    QHash<ListEntryEnvolued *,ConnexionInfo *> serverConnexion;
    SelectedServer selectedServer;//no selected if unique_code empty
    AddOrEditServer *addServer;
    Login *login;

    QNetworkAccessManager qnam;
    QNetworkReply *reply;

    CustomButton *server_add;
    CustomButton *server_remove;
    CustomButton *server_edit;
    CustomButton *server_select;
    CustomButton *server_refresh;
    CustomButton *back;
    QGraphicsProxyWidget *serverListProxy;
    QGraphicsTextItem *warning;
    QWidget *scrollAreaWidgetContentsServer;
    QWidget *serverWidget;
    QLabel *serverEmpty;
signals:
    void backMain();
    void setAbove(QGraphicsItem *widget);//first plan popup
    void connectToServer(ConnexionInfo connexionInfo,QString login,QString pass);
};

#endif // MULTI_H
