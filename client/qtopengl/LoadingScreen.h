#ifndef LOADINGSCREEN_H
#define LOADINGSCREEN_H

#include <QGraphicsItem>
#include <QTimer>
#include "CCWidget.h"
#include "CCprogressbar.h"

namespace Ui {
class LoadingScreen;
}

class LoadingScreen : public QObject, public QGraphicsItem
{
    Q_OBJECT

public:
    explicit LoadingScreen();
    ~LoadingScreen();
    void progression(uint32_t size, uint32_t total);
    void setText(QString text);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
    QRectF boundingRect() const override;
protected:
    void canBeChanged();
    void dataIsParsed();
private:
    Ui::LoadingScreen *ui;
    CCWidget *widget;
    QGraphicsPixmapItem *teacher;
    QGraphicsTextItem *info;
    CCprogressbar *progressbar;
    QGraphicsTextItem *version;
    QTimer timer;
    bool doTheNext;
signals:
    void finished();
};

#endif // LOADINGSCREEN_H
