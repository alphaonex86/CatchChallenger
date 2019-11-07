#ifndef LOADINGSCREEN_H
#define LOADINGSCREEN_H

#include <QGraphicsItem>
#include <QTimer>
#include "CCWidget.h"
#include "CCprogressbar.h"

namespace Ui {
class LoadingScreen;
}

class LoadingScreen : public QGraphicsItem
{
    Q_OBJECT

public:
    explicit LoadingScreen();
    ~LoadingScreen();
    void progression(uint32_t size, uint32_t total);
    void setText(QString text);
protected:
    void canBeChanged();
    void dataIsParsed();
private:
    Ui::LoadingScreen *ui;
    CCWidget *widget;
    QGraphicsSimpleTextItem *teacher;
    QGraphicsSimpleTextItem *info;
    CCprogressbar *progressbar;
    QGraphicsSimpleTextItem *version;
    QTimer timer;
    bool doTheNext;
signals:
    void finished();
};

#endif // LOADINGSCREEN_H
