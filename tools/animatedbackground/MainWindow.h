#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "CCBackground.h"
#include "CustomButton.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    void resizeEvent(QResizeEvent *event) override;
private:
    Ui::MainWindow *ui;
    CCBackground *m_CCBackground;
    CustomButton *p;
};

#endif // MAINWINDOW_H
