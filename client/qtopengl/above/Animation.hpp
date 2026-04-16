#ifndef Animation_H
#define Animation_H

#include <QObject>
#include <QTimer>
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../ImagesStrechMiddle.hpp"
#include "../CustomText.hpp"
#include "../../../general/base/GeneralStructures.hpp"

class ConnexionManager;

class Animation : public ScreenInput
{
    Q_OBJECT
public:
    explicit Animation();
    ~Animation();

    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral) override;
    void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral) override;

    void startEvolution(const uint16_t monsterIdFrom, const uint16_t monsterIdTo);
private slots:
    void animationTick();
    void onFinishClicked();
signals:
    void setAbove(ScreenInput *widget);
    void animationFinished();
private:
    ImagesStrechMiddle *wdialog;
    CustomButton *finishButton;
    CustomText *title;
    QGraphicsPixmapItem label;

    QGraphicsPixmapItem *monsterImageFrom;
    QGraphicsPixmapItem *monsterImageTo;
    QGraphicsTextItem *evolutionText;

    QPixmap pixFrom;
    QPixmap pixTo;

    QTimer animTimer;
    int animStep;
    static const int ANIM_TOTAL_STEPS=60;
};

#endif // Animation_H
