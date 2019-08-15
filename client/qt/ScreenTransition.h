#ifndef SCREENTRANSITION_H
#define SCREENTRANSITION_H

#include <QWidget>
#include <QStackedWidget>
#include "../qt/CCBackground.h"
#include "../qt/LoadingScreen.h"

class ScreenTransition : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenTransition();
    ~ScreenTransition();
    void setBackground(QWidget *widget);
    void setForeground(QWidget *widget);
protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void toMainScreen();
private:
    CCBackground b;
    LoadingScreen l;
    QStackedWidget *m_backgroundStack;
    QStackedWidget *m_foregroundStack;
};

#endif // SCREENTRANSITION_H
