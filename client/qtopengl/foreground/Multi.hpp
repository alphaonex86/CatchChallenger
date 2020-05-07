#ifndef MULTI_H
#define MULTI_H

#include <QNetworkAccessManager>
#include <QString>
#include <QHash>
#include <QSet>
#include <vector>
#include "../CCWidget.hpp"
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../ConnexionInfo.hpp"
#include "../CCScrollZone.hpp"

class ListEntryEnvolued;
class AddOrEditServer;
class Login;
class MultiItem;
class QNetworkReply;

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
    explicit Multi();
    ~Multi();
    void displayServerList();
    void server_add_clicked();
    void server_add_finished();
    void server_edit_clicked();
    void server_edit_finished();
    void server_select_clicked();
    void server_select_finished();
    void server_remove_clicked();
    void saveConnexionInfoList();
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
    void mousePressEventXY(const QPointF &p,bool &pressValidated) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated) override;
private:
    std::vector<ConnexionInfo> temp_customConnexionInfoList,temp_xmlConnexionInfoList,mergedConnexionInfoList;
    QList<MultiItem *> serverConnexion;
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
    QGraphicsTextItem *warning;

    CCWidget *wdialog;
    QGraphicsTextItem *serverEmpty;
    CCScrollZone *scrollZone;
signals:
    void backMain();
    void setAbove(ScreenInput *widget);//first plan popup
    void connectToServer(ConnexionInfo connexionInfo,QString login,QString pass);
};

#endif // MULTI_H
