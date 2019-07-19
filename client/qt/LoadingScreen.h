#ifndef LOADINGSCREEN_H
#define LOADINGSCREEN_H

#include <QWidget>

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
};

#endif // LOADINGSCREEN_H
