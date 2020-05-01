#ifndef LOADINGSCREEN_H
#define LOADINGSCREEN_H

#include <QTimer>
#include <QDateTime>
#include "../CCWidget.hpp"
#include "../CCprogressbar.hpp"
#include "../ScreenInput.hpp"

namespace Ui {
class LoadingScreen;
}

class LoadingScreen : public QObject, public ScreenInput
{
    Q_OBJECT

public:
    explicit LoadingScreen();
    ~LoadingScreen();
    void progression(uint32_t size, uint32_t total);
    void setText(QString text);

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *widget) override;
    QRectF boundingRect() const override;
protected:
    void canBeChanged();
    void dataIsParsed();
    void updateProgression();

    void newLanguage();
private:
    Ui::LoadingScreen *ui;
    CCWidget *widget;
    QGraphicsPixmapItem *teacher;
    QGraphicsTextItem *info;
    CCprogressbar *progressbar;
    QGraphicsTextItem *version;
    QTimer timer;
    QTimer slowDownProgressionTimer;
    uint8_t lastProgression;
    uint8_t timerProgression;
    bool doTheNext;
signals:
    void finished();
};

#endif // LOADINGSCREEN_H
