#ifndef LOADINGSCREEN_H
#define LOADINGSCREEN_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include "CCWidget.h"
#include "CCprogressbar.h"
#include "GameLoader.h"

namespace Ui {
class LoadingScreen;
}

class LoadingScreen : public QWidget
{
    Q_OBJECT

public:
    explicit LoadingScreen(QWidget *parent = nullptr);
    ~LoadingScreen();
    void progression(uint32_t size, uint32_t total);
protected:
    void resizeEvent(QResizeEvent *event) override;
    void canBeChanged();
    void dataIsParsed();
private:
    Ui::LoadingScreen *ui;
    CCWidget *widget;
    QHBoxLayout *horizontalLayout;
    QLabel *teacher;
    QLabel *info;
    CCprogressbar *progressbar;
    QLabel *version;
    QTimer timer;
    bool doTheNext;
signals:
    void finished();
};

#endif // LOADINGSCREEN_H
