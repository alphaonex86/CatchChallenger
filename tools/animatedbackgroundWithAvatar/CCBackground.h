#ifndef CCBackground_H
#define CCBackground_H

#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <chrono>
#include <QGraphicsItem>
#include <QTime>

#define SPEED 8

class CCBackground : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    CCBackground();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *widget) override;
    void startAnimation();
    void stopAnimation();
private:
    QPixmap cloud,grass,sun,treeback,treefront;
    void scalePix(QPixmap &pix,unsigned int zoom);
    void grassSlot();
    void treebackSlot();
    void treefrontSlot();
    void cloudSlot();
    unsigned int getTargetZoom(QWidget *widget);
    void updateSlot();
    QRectF boundingRect() const override;

    unsigned int zoom;
    int grassMove,treebackMove,treefrontMove,cloudMove;
    int floorMove;
    QTimer grassTimer,treebackTimer,treefrontTimer,cloudTimer;
    QTimer updateTimer;
    std::vector<unsigned int> results;

    QPixmap player1,player2;
    int player1Frame;
    int animationStep;

    QPixmap dialog1,dialog2;
    QTimer pauseBeforeDisplayDialog;
    QTimer pauseBeforeRemovePerSyllableDialog;
    std::string dialog1Text,dialog2Text;
    static int count_partial_letter(std::string s);
    void showDialogSlot();
    void hideDialogSlot();
    std::vector<std::string> dialogs;
    QGraphicsTextItem text;
};

#endif // PROGRESSBARDARK_H
