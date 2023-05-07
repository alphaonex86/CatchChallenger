#ifndef Battle_H
#define Battle_H

#include "../ScreenInput.hpp"
#include "../../qtmaprender/Map_client.hpp"
#include "../ConnexionManager.hpp"
#include <QObject>
class CCprogressbar;
class ImagesStrechMiddle;
class CustomButton;
class QGraphicsProxyWidget;
class QListWidget;
class QListWidgetItem;

class Battle : public ScreenInput
{
    Q_OBJECT
public:
    explicit Battle();
    ~Battle();
    static Battle *battle;
    void newLanguage();
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void stackedWidgetFightBottomBarsetCurrentWidget(int index);

    void on_monsterList_itemActivated(uint8_t monsterPosition);
    bool check_monsters();
    void init_environement_display(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y);
    void setVar(ConnexionManager *connexionManager);
    void init_current_monster_display(CatchChallenger::PlayerMonster *fightMonster);
public slots:
    void on_pushButtonFightEnterNext_clicked();
/*
    void on_pushButtonFightEnterNext_clicked();
    void on_toolButtonFightQuit_clicked();
    void on_pushButtonFightAttack_clicked();
    void on_pushButtonFightMonster_clicked();
    void on_pushButtonFightAttackConfirmed_clicked();
    void on_pushButtonFightReturn_clicked();
    void on_listWidgetFightAttack_itemSelectionChanged();
    void on_listWidgetFightAttack_itemActivated(QListWidgetItem *item);*/
private:
    ImagesStrechMiddle *frameFightBottom;
    QList<QGraphicsPixmapItem *> bottomBuff;
    QGraphicsTextItem *labelFightBottomName;
    CCprogressbar *progressBarFightBottomHP;

    ImagesStrechMiddle *frameFightTop;
    QList<QGraphicsPixmapItem *> topBuff;
    QGraphicsTextItem *labelFightTopName;
    CCprogressbar *progressBarFightTopHP;

    QGraphicsPixmapItem *labelFightBackground;
    QPixmap labelFightBackgroundPix;
    QGraphicsPixmapItem *labelFightForeground;
    QPixmap labelFightForegroundPix;
    QGraphicsPixmapItem *labelFightPlateformBottom;
    QPixmap labelFightPlateformBottomPix;
    QGraphicsPixmapItem *labelFightPlateformTop;
    QPixmap labelFightPlateformTopPix;

    QGraphicsPixmapItem *labelFightMonsterAttackBottom;
    QGraphicsPixmapItem *labelFightMonsterAttackTop;
    QGraphicsPixmapItem *labelFightMonsterBottom;
    QGraphicsPixmapItem *labelFightMonsterTop;
    QGraphicsPixmapItem *labelFightTrap;

    ImagesStrechMiddle *stackedWidgetFightBottomBar;

    QGraphicsTextItem *labelFightEnter;
    CustomButton *pushButtonFightEnterNext;

    CustomButton *pushButtonFightAttack;
    CustomButton *pushButtonFightBag;
    CustomButton *pushButtonFightMonster;
    CustomButton *toolButtonFightQuit;

    CustomButton *pushButtonFightAttackConfirmed;
    CustomButton *pushButtonFightReturn;
    QGraphicsTextItem *labelFightAttackDetails;
    QGraphicsProxyWidget *listWidgetFightAttackProxy;
    QListWidget *listWidgetFightAttack;

    int m_stackedWidgetFightBottomBarIndex=0;

    bool escape;
    bool escapeSuccess;
    ConnexionManager *connexionManager;
};

#endif // MULTI_H
