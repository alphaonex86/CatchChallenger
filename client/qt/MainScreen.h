#ifndef MAINSCREEN_H
#define MAINSCREEN_H

#include <QWidget>
#include <QLabel>
#include "CustomButton.h"
#include "CCWidget.h"

namespace Ui {
class MainScreen;
}

class MainScreen : public QWidget
{
    Q_OBJECT
public:
    explicit MainScreen(QWidget *parent = nullptr);
    ~MainScreen();
private:
    Ui::MainScreen *ui;
    //QLabel *logo;
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
};

#endif // MAINSCREEN_H
