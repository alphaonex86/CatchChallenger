#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QTime>
#include <QGraphicsSimpleTextItem>
#include "CCBackground.h"
#include "CustomButton.h"

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
    CCBackground *m_CCBackground;
    QGraphicsScene *mScene;

    uint8_t waitRenderTime;
    QTimer timerRender;
    QTime timeRender;
    uint16_t frameCounter;
    QTimer startRender;
private:
    void startRenderSlot();
    void render();
    void paintEvent(QPaintEvent * event) override;
signals:
    void newFPSvalue(const unsigned int FPS);
};

#endif // MAINWINDOW_H
