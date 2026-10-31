#ifndef Animation_H
#define Animation_H

#include <QObject>
#include <QElapsedTimer>
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

    //canCancel false for an item evolution: the item is already used
    void startEvolution(const uint16_t monsterIdFrom, const uint16_t monsterIdTo, const bool canCancel);
private slots:
    void animationTick();
    void onButtonClicked();
signals:
    void setAbove(ScreenInput *widget);
    //the animation went to the end: confirm it to the server; never emitted when the player cancels
    void evolved();
    void closed();
private:
    enum State : uint8_t
    {
        State_Evolving,
        State_Evolved,
        State_Canceled
    };
    void showEnd(const State &state);
    void updateMonsterImage();
    static QString monsterName(const uint16_t monsterId);
    static QPixmap monsterFront(const uint16_t monsterId);

    ImagesStrechMiddle *wdialog;
    CustomButton *button;
    CustomText *title;
    QGraphicsPixmapItem label;

    QGraphicsPixmapItem *monsterImage;
    QGraphicsTextItem *evolutionText;

    QPixmap pixFrom;
    QPixmap pixTo;
    QString nameFrom;
    QString nameTo;
    //what monsterImage holds, to rescale only on change
    bool imageIsTo;
    int imageSize;
    int monsterSize;

    QTimer animTimer;
    QElapsedTimer elapsed;
    State state;
};

#endif // Animation_H
