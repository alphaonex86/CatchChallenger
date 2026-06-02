#ifndef TILEDEDUPLICATOR_HPP
#define TILEDEDUPLICATOR_HPP

#include <QWidget>
#include <QString>
#include <QHash>
#include <QPair>
#include <QSharedPointer>
#include <QRgb>

#include <vector>
#include <unordered_set>
#include <cstdint>

class QLabel;
class QPushButton;
class QTimer;

namespace Tiled {
class Map;
class Layer;
class Tileset;
class Tile;
}

// One 16x16 tile extracted from a tileset image. Identified by the .tsx file
// it belongs to (canonical absolute path) plus its local tile id. Pixels are
// kept as a flat row-major ARGB32 buffer for cache-friendly comparison.
struct TileEntry
{
    QString tsx;               // canonical .tsx path
    int id;                    // local tile id inside that tileset
    std::vector<QRgb> px;      // kTilePx pixels, row-major, Format_ARGB32
    quint64 hash;              // FNV-1a over px (exact-equality fast path)
    quint64 alpha_sum;         // sum of alpha over all pixels (opacity metric)
    int merged_into;           // index of the kept tile, or -1 when active/root
};

// Deduplicate visually identical/similar 16x16 tiles across every tileset in a
// datapack, then rewrite all .tmx maps so cells/objects that referenced a
// merged-away tile point at the kept tile instead.
//
// The whole pipeline is driven from the Qt event loop so the window stays
// responsive: a repeating zero-delay timer steps the O(n^2) comparison in
// bounded batches, pausing only to ask the user about SIMILAR pairs.
class TileDeduplicator : public QWidget
{
    Q_OBJECT
public:
    // batch=true runs headless: auto-merge IDENTICAL tiles, auto-keep SIMILAR
    // ones (they need a human decision) and never open a prompt. Used for
    // scripted/CI runs; the interactive window is the default (batch=false).
    explicit TileDeduplicator(const QString &datapackPath, bool batch = false, QWidget *parent = nullptr);
    ~TileDeduplicator() override;

private slots:
    void loadAndStart();       // first event-loop tick: load tilesets, begin
    void onStep();             // process a bounded batch of comparisons
    void onSkipClicked();      // user kept both tiles of a SIMILAR pair
    void onMergeClicked();     // user merged a SIMILAR pair

private:
    enum CompareLevel : std::uint8_t { Identical, Similar, Different };

    bool loadTilesets();
    static CompareLevel comparePixels(const TileEntry &a, const TileEntry &b);
    void mergePair(int a, int b, bool *sourceLost);
    int findRoot(int idx) const;
    bool identityLess(const TileEntry &a, const TileEntry &b) const;
    void advanceSource();
    void updateProgress();
    void showPrompt(int a, int b);
    void finish();

    QPixmap renderPreview(const TileEntry &t) const;

    // .tmx rewriting
    void applyRemaps();
    void remapOneMap(const QString &fileName);
    void remapLayers(const QList<Tiled::Layer *> &layers, Tiled::Map *map);
    Tiled::Tile *keeperTileForMap(Tiled::Map *map, const QString &tsx, int id);

    QString datapack_path_;
    bool batch_;
    std::vector<TileEntry> tiles_;
    int i_;
    int j_;
    std::unordered_set<quint64> skipped_sources_;

    // Final loser-tile -> kept-tile remap, key = "<canonical tsx>#<id>".
    QHash<QString, QPair<QString, int> > remap_;

    // Per-map scratch, reset in remapOneMap().
    QHash<QString, QSharedPointer<Tiled::Tileset> > ts_by_canon_;
    QHash<const Tiled::Tileset *, QString> canon_by_ts_;
    bool map_changed_;

    int identical_merges_;
    int similar_merges_;
    int skips_;
    int maps_rewritten_;
    int maps_skipped_;

    QTimer *step_timer_;
    QLabel *status_label_;
    QWidget *decision_widget_;
    QLabel *prompt_label_;
    QLabel *preview_a_;
    QLabel *preview_b_;
    QLabel *info_a_;
    QLabel *info_b_;
    QPushButton *skip_button_;
    QPushButton *merge_button_;
};

#endif // TILEDEDUPLICATOR_HPP
