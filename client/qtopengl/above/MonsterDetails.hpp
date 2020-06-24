#ifndef MONSTERDETAILS_H
#define MONSTERDETAILS_H

#include "../ScreenInput.hpp"
#include "../../../general/base/GeneralStructures.hpp"
#include <QGraphicsProxyWidget>
#include <QListWidget>

class CustomButton;
class CustomText;
class ProgressBarPixel;

class MonsterDetails : public ScreenInput
{
    Q_OBJECT
public:
    explicit MonsterDetails();
    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    virtual void keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral);
    virtual void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral);
    void setVar(const CatchChallenger::PlayerMonster &monster);
private slots:
    void newLanguage();
private:
    CustomButton *quit;
    QGraphicsTextItem *details;

    QGraphicsPixmapItem *monsterDetailsImage;

    QGraphicsProxyWidget *proxy;
    QListWidget *monsterDetailsSkills;

    QGraphicsPixmapItem *monsterDetailsCatched;
    CustomText *monsterDetailsName;
    QGraphicsTextItem *monsterDetailsLevel;
    QGraphicsTextItem *hp_text;
    ProgressBarPixel *hp_bar;
    QGraphicsTextItem *xp_text;
    ProgressBarPixel *xp_bar;
};

#endif // MONSTERDETAILS_H
