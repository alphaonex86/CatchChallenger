#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QWidget>
#include "CCWidget.h"
#include "ScreenInput.h"
#include "CustomButton.h"

class OptionsDialog : public QObject, public ScreenInput
{
    Q_OBJECT
public:
    explicit OptionsDialog();
    ~OptionsDialog();
    void resizeEvent(QResizeEvent * e);
    QRectF boundingRect() const override;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p) override;
    void mouseReleaseEventXY(const QPointF &p) override;
private:
    CCWidget *wdialog;
    CustomButton *quit;
    QGraphicsPixmapItem label;
    int x,y;
signals:
    void quitOption();
};

#endif // OPTIONSDIALOG_H
