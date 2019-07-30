#ifndef LOADINGSCREEN_H
#define LOADINGSCREEN_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include "CCWidget.h"
#include "CCprogressbar.h"

namespace Ui {
class LoadingScreen;
}

class LoadingScreen : public QWidget
{
    Q_OBJECT

public:
    explicit LoadingScreen(QWidget *parent = nullptr);
    ~LoadingScreen();
protected:
    void resizeEvent(QResizeEvent *event) override;
private:
    Ui::LoadingScreen *ui;
    CCWidget *widget;
    QHBoxLayout *horizontalLayout;
    QLabel *teacher;
    QLabel *info;
    CCprogressbar *progressbar;
};

#endif // LOADINGSCREEN_H
