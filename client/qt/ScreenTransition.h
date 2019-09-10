#ifndef SCREENTRANSITION_H
#define SCREENTRANSITION_H

#include <QWidget>
#include <QStackedWidget>
#include "../qt/CCBackground.h"
#include "../qt/LoadingScreen.h"
#include "OptionsDialog.h"
#include "MainScreen.h"

class ScreenTransition : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenTransition();
    ~ScreenTransition();
    void setBackground(QWidget *widget);
    void setForeground(QWidget *widget);
    void setAbove(QWidget *widget);//first plan popup
protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void toMainScreen();
    void openOptions();
    void closeOptions();
private:
    CCBackground b;
    LoadingScreen l;
    QStackedWidget *m_backgroundStack;
    QStackedWidget *m_foregroundStack;
    QStackedWidget *m_aboveStack;
    OptionsDialog *o;
    MainScreen *m;
};

#endif // SCREENTRANSITION_H
