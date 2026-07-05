#ifndef MonsterSelect_H
#define MonsterSelect_H

#include <QObject>
#include <QListWidget>
#include <QGraphicsProxyWidget>
#include "../../ImagesStrechMiddle.hpp"
#include "../../ScreenInput.hpp"
#include "../../CustomButton.hpp"
#include "../../CustomText.hpp"

class ConnexionManager;

//A team-monster picker used whenever an action needs a target monster (use an
//item on a monster, evolve, learn a skill, switch in fight, trade a monster).
//Content-sized floating popup in the qtopengl idiom; emits useMonster(position)
//on pick and canceled() on quit so OverMapLogic can route to objectSelection().
class MonsterSelect : public ScreenInput
{
    Q_OBJECT
public:
    explicit MonsterSelect();
    ~MonsterSelect();

    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral) override;
    void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral) override;
    void setVar(ConnexionManager *connexionManager);
private slots:
    void newLanguage();
    void onCancel();
    void onSelectClicked();
    void on_list_itemSelectionChanged();
private:
    ImagesStrechMiddle *wdialog;
    CustomButton *quit;
    CustomText *title;
    QGraphicsPixmapItem label;
    QListWidget *monsterList;
    QGraphicsProxyWidget *listProxy;
    CustomButton *selectButton;
    ConnexionManager *connexionManager;
signals:
    void setAbove(ScreenInput *widget);//first plan popup
    void useMonster(uint8_t monsterPosition);
    void canceled();
};

#endif // MonsterSelect_H
