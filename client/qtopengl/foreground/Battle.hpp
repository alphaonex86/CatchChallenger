#ifndef Battle_H
#define Battle_H

#include "../ScreenInput.hpp"
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
    void newLanguage();
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void stackedWidgetFightBottomBarsetCurrentWidget(int index);
public slots:
    void on_pushButtonFightEnterNext_clicked();
    void on_toolButtonFightQuit_clicked();
    void on_pushButtonFightAttack_clicked();
    void on_pushButtonFightMonster_clicked();
    void on_pushButtonFightAttackConfirmed_clicked();
    void on_pushButtonFightReturn_clicked();
    void on_listWidgetFightAttack_itemSelectionChanged();
    void on_listWidgetFightAttack_itemActivated(QListWidgetItem *item);
private:
    QGraphicsPixmapItem *frameFightBottom;
    QList<QGraphicsPixmapItem *> bottomBuff;
    QGraphicsTextItem *labelFightBottomLevel;
    QGraphicsTextItem *labelFightBottomName;
    CCprogressbar *progressBarFightBottomHP;

    QGraphicsPixmapItem *frameFightTop;
    QList<QGraphicsPixmapItem *> topBuff;
    QGraphicsTextItem *labelFightTopLevel;
    QGraphicsTextItem *labelFightTopName;
    CCprogressbar *progressBarFightTopHP;

    QGraphicsPixmapItem *labelFightBackground;
    QPixmap labelFightBackgroundPix;
    QGraphicsPixmapItem *labelFightForeground;
    QPixmap labelFightForegroundPix;

    QGraphicsPixmapItem *labelFightMonsterAttackBottom;
    QGraphicsPixmapItem *labelFightMonsterAttackTop;
    QGraphicsPixmapItem *labelFightMonsterBottom;
    QGraphicsPixmapItem *labelFightMonsterTop;
    QGraphicsPixmapItem *labelFightPlateformBottom;
    QGraphicsPixmapItem *labelFightPlateformTop;
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
};

#endif // MULTI_H
