#ifndef TILESETTAGGER_TILESETVIEW_HPP
#define TILESETTAGGER_TILESETVIEW_HPP

// The tileset canvas: paints the .tsx image at an integer zoom with a tile grid,
// tints tagged tiles by category colour, highlights untagged-non-empty tiles in
// red (the owner's "reminder"), and lets the user rubber-band a rectangle of
// tiles to tag.  Pure painting + mouse; the data lives in TagModel.

#include <QWidget>

class TagModel;

class TilesetView : public QWidget {
    Q_OBJECT
public:
    explicit TilesetView(QWidget *parent=nullptr);
    void setModel(TagModel *model);     // not owned
    void refresh();                     // repaint after the model changed
    void setShowStates(bool on);
    void setZoom(int z);
    int zoom() const;
    void selectCell(int col,int row);   // programmatic selection (jump-to-untagged)
    QRect cellPixelRect(int col,int row) const;
    QSize sizeHint() const override;

signals:
    void selectionChanged(int col0,int row0,int col1,int row1,int tileCount);
    // emitted once the drag ends (or on programmatic selectCell) — use this for
    // the heavy map-usage scan so it does not run on every drag-move.
    void selectionFinished(int col0,int row0,int col1,int row1,int tileCount);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    TagModel *model_;
    int zoom_;
    bool showStates_;
    bool selecting_;
    bool hasSelection_;
    int selCol0_;
    int selRow0_;
    int selCol1_;
    int selRow1_;
    void cellAt(const QPoint &pt,int &col,int &row) const;
    void emitSelection(bool finished);
};

#endif // TILESETTAGGER_TILESETVIEW_HPP
