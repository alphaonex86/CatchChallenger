#ifndef TILESETTAGGER_MAPUSAGEVIEW_HPP
#define TILESETTAGGER_MAPUSAGEVIEW_HPP

// Shows a rendered map scaled to fit, with the selected tile group's usage cells
// merged into rectangles and outlined with ANIMATED marching-ants borders — the
// "show me where it is used, animated" half of the owner's two-step tagging.

#include <QImage>
#include <QRect>
#include <QWidget>
#include <vector>

class QTimer;

class MapUsageView : public QWidget {
    Q_OBJECT
public:
    explicit MapUsageView(QWidget *parent=nullptr);
    void setUsage(const QImage &mapImage,const std::vector<QPoint> &cells,int tileW,int tileH);
    void clearUsage();
    QPoint firstHighlightCenter() const;   // pixel centre of the first used cell (native coords)
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onTick();

private:
    QImage map_;
    std::vector<QRect> rects_;   // merged usage rectangles, in TILE coords
    int tileW_;
    int tileH_;
    int phase_;
    QTimer *timer_;
};

#endif // TILESETTAGGER_MAPUSAGEVIEW_HPP
