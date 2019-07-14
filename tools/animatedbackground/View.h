#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QTimer>

class View : public QGraphicsView
{
    Q_OBJECT
public:
    explicit View(QWidget *parent = 0);
private:
    QGraphicsPixmapItem cloud,grass,sun,treeback,treefront;
    void scalePix(QPixmap &pix,unsigned int zoom);
    void grassSlot();
    void treebackSlot();
    void treefrontSlot();
    unsigned int getTargetZoom();
    void updateCustom();
    //void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *event) override;

    unsigned int zoom;
    int grassMove,treebackMove,treefrontMove;
    QTimer grassTimer,treebackTimer,treefrontTimer;
    QTimer updateTimer;

    QGraphicsScene scene;
};
