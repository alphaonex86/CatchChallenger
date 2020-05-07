#ifndef CharacterList_H
#define CharacterList_H

#include <QString>
#include <QHash>
#include <QSet>
#include <vector>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "../CCWidget.hpp"
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../ConnexionInfo.hpp"
#include "../CCScrollZone.hpp"

class AddCharacter;

class CharacterList : public QObject, public ScreenInput
{
    Q_OBJECT
public:
    explicit CharacterList();
    ~CharacterList();
    void displayServerList();
    void server_add_clicked();
    void server_add_finished();
    void server_select_clicked();
    void server_select_finished();
    void server_remove_clicked();
    void newLanguage();
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated) override;
    void connectToSubServer(const int indexSubServer);
private:
    AddCharacter *addCharacter;

    CustomButton *server_add;
    CustomButton *server_remove;
    CustomButton *server_select;
    CustomButton *back;
    QGraphicsTextItem *warning;

    CCWidget *wdialog;
signals:
    void backSubServer();
    void setAbove(ScreenInput *widget);//first plan popup
    void selectCharacter(const int indexSubServer,const int indexCharacter);
};

#endif // MULTI_H
