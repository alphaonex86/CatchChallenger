#ifndef MAINSCREEN_H
#define MAINSCREEN_H

#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include "CustomButton.h"
#include "CCWidget.h"
#include "CCTitle.h"

namespace Ui {
class MainScreen;
}

class MainScreen : public QWidget
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
    CCTitle *title;
    CustomButton *updateButton;
    CustomButton *solo;
    CustomButton *multi;
    CustomButton *options;
    CustomButton *facebook;
    CustomButton *website;
    CCWidget *news;
    QLabel *newsText;
    QLabel *newsWait;
    CustomButton *newsUpdate;
    QHBoxLayout *verticalLayoutNews;
protected:
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
};

#endif // MAINSCREEN_H
