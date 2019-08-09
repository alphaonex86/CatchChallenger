#ifndef MAINSCREEN_H
#define MAINSCREEN_H

#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QPushButton>

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

    QPushButton *updateButton;
    QPushButton *solo;
    QPushButton *multi;
    QPushButton *options;
    QPushButton *facebook;
    QPushButton *website;
protected:
    void resizeEvent(QResizeEvent *) override;
};

#endif // MAINSCREEN_H
