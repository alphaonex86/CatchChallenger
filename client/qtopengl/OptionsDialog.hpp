#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QWidget>
#include "CCWidget.hpp"
#include "ScreenInput.hpp"
#include "CustomButton.hpp"

class OptionsDialog : public QObject, public ScreenInput
{
    Q_OBJECT
public:
    explicit OptionsDialog();
    ~OptionsDialog();
    void resizeEvent(QResizeEvent * e);
    QRectF boundingRect() const override;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated) override;
    void mouseMoveEventXY(const QPointF &p, bool &pressValidated) override;
private:
    CCWidget *wdialog;
    CustomButton *quit;
    QGraphicsPixmapItem label;
    int x,y;
signals:
    void quitOption();
};

#endif // OPTIONSDIALOG_H
