#ifndef DEUBGDIALOG_H
#define DEUBGDIALOG_H

#include <QWidget>
#include <QComboBox>
#include <QTime>
#include "../ImagesStrechMiddle.hpp"
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../CustomText.hpp"
#include "../CCGraphicsTextItem.hpp"

class DebugDialog : public QObject, public ScreenInput
{
    Q_OBJECT
public:
    explicit DebugDialog();
    ~DebugDialog();
    void resizeEvent(QResizeEvent * e);
    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
private:
    ImagesStrechMiddle *wdialog;
    CustomButton *quit;
    CustomText *title;
    QGraphicsPixmapItem label;

    CCGraphicsTextItem *debugText;
    bool debugIsShow;
    QTime lastUpdate;
signals:
    void removeAbove();
};

#endif // OPTIONSDIALOG_H
