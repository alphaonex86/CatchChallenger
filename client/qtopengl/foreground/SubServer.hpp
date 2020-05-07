#ifndef SubServer_H
#define SubServer_H

#include <QString>
#include "../CCWidget.hpp"
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../../../general/base/GeneralStructures.hpp"
#include "../../qt/Api_client_real.hpp"

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
    void displayServerList();
    void server_select_clicked();
    void newLanguage();
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated) override;
    void logged(std::vector<CatchChallenger::ServerFromPoolForDisplay> serverOrdenedList);
private:
    std::vector<CatchChallenger::ServerFromPoolForDisplay> serverOrdenedList;
    CustomButton *server_select;
    CustomButton *back;
    QGraphicsTextItem *warning;

    CCWidget *wdialog;
signals:
    void backMulti();
    void setAbove(ScreenInput *widget);//first plan popup
    void connectToSubServer(const int indexSubServer);
};

#endif // MULTI_H
