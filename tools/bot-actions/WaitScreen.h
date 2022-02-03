#ifndef WAITSCREEN_H
#define WAITSCREEN_H

#include <QWidget>
#include <QTimer>

namespace Ui {
class WaitScreen;
}

class WaitScreen : public QWidget
{
    Q_OBJECT

public:
    explicit WaitScreen(QWidget *parent = 0);
    ~WaitScreen();
    void updateWaitScreen();
private:
    Ui::WaitScreen *ui;
    QTimer updateWaitScreenTimer;
};

#endif // WAITSCREEN_H
