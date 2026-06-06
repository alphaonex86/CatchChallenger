#ifndef TILESETTAGGER_TILESETVIEW_HPP
#define TILESETTAGGER_TILESETVIEW_HPP

// The tileset canvas: paints the .tsx image at a (fractional) zoom with a tile grid,
// tints tagged tiles by category colour, highlights untagged-non-empty tiles in
// red (the owner's "reminder"), and lets the user select tiles to tag.  Selection
// is a SET of cells (not a single rectangle), so a building whose tiles are
// interleaved with another object's can be picked exactly: plain drag REPLACES,
// Ctrl/Shift+drag ADDS a rectangle, Ctrl/Shift+click TOGGLES one cell.
// Pure painting + mouse; the data lives in TagModel.

#include <QWidget>
#include <set>
#include <vector>

class TagModel;
class QPainter;
class QTimer;

class TilesetView : public QWidget {
    Q_OBJECT
public:
    explicit TilesetView(QWidget *parent=nullptr);
    void setModel(TagModel *model);     // not owned
    void refresh();                     // repaint after the model changed
    void setShowStates(bool on);
    void setShowGroups(bool on);  // REVIEW mode: category-colour fills + animated group outlines
    void setZoom(double z);     // fractional so the sheet can fill the window AND shrink to fit
    double zoom() const;
    void selectCell(int col,int row);          // programmatic single selection (replaces)
    std::vector<int> selectedTiles() const;    // the selected tile ids, ascending
    bool hasSelection() const;
    void selectionBounds(int &c0,int &r0,int &c1,int &r1) const;  // bbox of the selection
    QRect cellPixelRect(int col,int row) const;
    QSize sizeHint() const override;

signals:
    void selectionChanged(int tileCount);
    // emitted once the drag/click ends (or on programmatic selectCell) — use this
    // for the heavy map-usage scan so it does not run on every drag-move.
    void selectionFinished(int tileCount);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onGroupTick();        // animate the review-mode group outlines

private:
    TagModel *model_;
    double zoom_;
    bool showStates_;
    bool showGroups_;          // review mode (category fills + animated outlines)
    QTimer *groupTimer_;
    int groupPhase_;
    void paintGroupReview(QPainter &p,double tw,double th,int cols,int n);
    bool selecting_;          // a drag is in progress
    bool additive_;           // Ctrl/Shift held at drag start -> add to the selection
    int dragC0_;              // live rubber-band rectangle while selecting_
    int dragR0_;
    int dragC1_;
    int dragR1_;
    std::set<int> selected_;  // committed selection: tile ids
    void cellAt(const QPoint &pt,int &col,int &row) const;
    void commitDrag();        // fold the live rubber-band into selected_
    void emitSelection(bool finished);
};

#endif // TILESETTAGGER_TILESETVIEW_HPP
