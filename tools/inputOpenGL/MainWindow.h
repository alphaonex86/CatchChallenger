#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QGraphicsView>
#include "CCTextLineInput.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QGraphicsView
{
    Q_OBJECT
public:
    explicit MainWindow();
    ~MainWindow();
protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
private:
    Ui::MainWindow *ui;
    CCTextLineInput *input;
    QGraphicsScene *mScene;
private:
    void render();
    void paintEvent(QPaintEvent * event) override;
};

#endif // MAINWINDOW_H
