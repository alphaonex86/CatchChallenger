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
    CustomButton *p;
    QGraphicsScene *mScene;
    QGraphicsTextItem* simpleText;
    QGraphicsPixmapItem *image;
    QGraphicsPixmapItem *imageText;
    CustomButton *button;

    uint8_t waitRenderTime;
    QTimer timerRender;
    QTime timeRender;
    uint16_t frameCounter;
    QTimer timerUpdateFPS;
    QTime timeUpdateFPS;
private:
    void render();
    void paintEvent(QPaintEvent * event) override;
    void updateFPS();
    void setTargetFPS(int targetFPS);
signals:
    void newFPSvalue(const unsigned int FPS);
};

#endif // MAINWINDOW_H
