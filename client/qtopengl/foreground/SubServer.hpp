#ifndef SubServer_H
#define SubServer_H

#include <QString>
#include <QTreeWidgetItem>
#include <QGraphicsProxyWidget>
#include "../ImagesStrechMiddle.hpp"
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../ConnexionManager.hpp"
#include "../../../general/base/GeneralStructures.hpp"
#include "../cc/Api_client_real.hpp"

class ListEntryEnvolued;
class AddOrEditServer;
class Login;
class MultiItem;

class SubServer : public QObject, public ScreenInput
{
    Q_OBJECT
public:
    explicit SubServer();
    ~SubServer();
    void server_select_clicked();
    void newLanguage();
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void logged(std::vector<CatchChallenger::ServerFromPoolForDisplay> serverOrdenedList,ConnexionManager *connexionManager);
    void itemSelectionChanged();
private:
    void addToServerList(CatchChallenger::LogicialGroup &logicialGroup, QTreeWidgetItem *item, const uint64_t &currentDate, const bool &fullView);
private:
    std::vector<CatchChallenger::ServerFromPoolForDisplay> serverOrdenedList;
    CustomButton *server_select;
    CustomButton *back;

    QTreeWidget *serverList;
    QGraphicsProxyWidget *serverListProxy;

    ImagesStrechMiddle *wdialog;
    uint32_t averagePlayedTime,averageLastConnect;
    std::unordered_map<uint8_t/*character group index*/,std::pair<uint8_t/*server count*/,uint8_t/*temp Index to display*/> > serverByCharacterGroup;

    QIcon icon_server_list_star1;
    QIcon icon_server_list_star2;
    QIcon icon_server_list_star3;
    QIcon icon_server_list_star4;
    QIcon icon_server_list_star5;
    QIcon icon_server_list_star6;
    QIcon icon_server_list_stat1;
    QIcon icon_server_list_stat2;
    QIcon icon_server_list_stat3;
    QIcon icon_server_list_stat4;
    QIcon icon_server_list_bug;
    std::vector<QIcon> icon_server_list_color;
    ConnexionManager *connexionManager;
signals:
    void backMulti();
    void connectToSubServer(const int indexSubServer);
};

#endif // MULTI_H
