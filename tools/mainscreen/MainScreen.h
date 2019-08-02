#ifndef MAINSCREEN_H
#define MAINSCREEN_H

#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include "CustomButton.h"
#include "CCWidget.h"

namespace Ui {
class MainScreen;
}

class MainScreen : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainScreen(QWidget *parent = nullptr);
    ~MainScreen();
    void updateWidgetPos();
private:
    Ui::MainScreen *ui;

    QLabel *update;
    QLabel *updateStar;
    QLabel *updateText;
    CustomButton *updateButton;
    CustomButton *solo;
    CustomButton *multi;
    CustomButton *options;
    CustomButton *facebook;
    CustomButton *website;
    CCWidget *news;
protected:
    void resizeEvent(QResizeEvent *) override;
};

#endif // MAINSCREEN_H
