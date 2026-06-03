#ifndef TILEDEDUPLICATOR_HPP
#define TILEDEDUPLICATOR_HPP

#include <QWidget>
#include <QString>
#include <QHash>
#include <QPair>
#include <QSharedPointer>
#include <QRgb>
#include <QByteArray>
#include <QPixmap>

#include <vector>
#include <unordered_set>
#include <cstdint>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

class QLabel;
class QPushButton;
class QTimer;
class QScrollArea;

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
    bool has_properties;       // tile carries custom properties: never auto-merge it
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
    explicit TileDeduplicator(const QString &datapackPath, bool batch = false, bool checkAll = false, QWidget *parent = nullptr);
    ~TileDeduplicator() override;

private slots:
    void loadAndStart();       // first event-loop tick: load tilesets, begin
    void onStep();             // process a bounded batch of comparisons
    void onSkipClicked();      // user kept both tiles of a SIMILAR pair
    void onBackClicked();      // revert the last decision and re-show its prompt
    void onSaveClicked();      // flush the current merges/skips to disk now
    void onAnimTick();         // blink the tile markers on the tileset views
    void onKeepLeftPick();     // manual replace: keep the left-sheet pick, drop the right
    void onKeepRightPick();    // manual replace: keep the right-sheet pick, drop the left

protected:
    // A click on a preview tile picks it as the keeper (drops the other);
    // resizing rescales the crisp previews so they grow when maximized.
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void onKeepA();            // keep the left tile (i_), drop the right (j_)
    void onKeepB();            // keep the right tile (j_), drop the left (i_)
    void mergeKeeping(int keeper, int loser);
    int previewZoom() const;   // current magnification from the preview size
    void pushUndo(quint64 skipHash);  // record the pre-decision state for BACK
    void onSheetClick(bool leftSheet, const QPoint &pos);  // pick a tile in a tileset view
    void manualReplace(int keeper, int loser);  // commit a manual keep/replace pick
    void updateSaveButton();   // SAVE enabled only when there are unsaved changes
    enum CompareLevel : std::uint8_t { Identical, Similar, Different };

    bool loadTilesets();
    // Presence-trigger layers (Collisions/Grass/Water/Ledges*) where a cell's
    // mere presence — even a fully transparent tile — carries gameplay meaning
    // (block / encounter / surf / ledge). Empty-tile clearing must NOT touch
    // these, or it would silently delete walls/encounters.
    static bool isPresenceLayer(const QString &name);
    static CompareLevel comparePixels(const TileEntry &a, const TileEntry &b);
    void mergePair(int a, int b, bool *sourceLost);
    int findRoot(int idx) const;
    bool identityLess(const TileEntry &a, const TileEntry &b) const;
    void advanceSource();
    void updateProgress();
    void showPrompt(int a, int b);
    void finish();

    QPixmap renderPreview(const TileEntry &t, int zoom) const;
    // Draw a tile's full tileset at 1x into a label, with a marker rectangle at
    // the tile's position (bright when highlightOn, for the blink), and scroll it
    // into view.
    void renderTilesetView(QLabel *label, QScrollArea *scroll, const TileEntry &t, bool highlightOn, int pickIdx) const;
    // The tileset sheet "with the changes done": the on-disk sheet with every tile
    // merged away this session repainted as its keeper, so a prompt previews the
    // deduplicated result instead of the raw on-disk art. Result is cached (built
    // lazily) and the cache is cleared on every merge.
    QPixmap effectiveSheet(const QString &tsx) const;

    // .tmx rewriting
    void applyRemaps();
    void remapOneMap(const QString &fileName);
    void remapLayers(const QList<Tiled::Layer *> &layers, Tiled::Map *map);
    Tiled::Tile *keeperTileForMap(Tiled::Map *map, const QString &tsx, int id);

    // Async map writing: a finished map is serialized to bytes on this thread,
    // then handed to a background writer thread, so disk I/O overlaps the parse +
    // remap of the next map ("write when no more changes into it").
    void startWriter();
    void enqueueWrite(const QString &fileName, const QByteArray &data);
    void writerLoop();
    void stopWriterAndJoin();

    QString datapack_path_;
    bool batch_;
    bool check_all_;           // also compare tiles within the same tileset
    std::vector<TileEntry> tiles_;
    int i_;
    int j_;
    std::unordered_set<quint64> skipped_sources_;
    // Fully-transparent ("empty") tiles, keyed "<canonical tsx>#<id>". They are
    // kept out of the O(n^2) compare; instead every visual tile-layer cell that
    // points at one is replaced by transparent space (an empty cell) on rewrite.
    std::unordered_set<std::string> transparent_tiles_;

    // Full tileset sheets (1x) and their column counts, kept so a SIMILAR prompt
    // can show each tile inside its tileset with a blinking position marker.
    QHash<QString, QPixmap> sheets_;
    QHash<QString, int> sheetCols_;
    // Cache of effectiveSheet() results, keyed by canonical tsx. In-memory only
    // (the .tsx/.png on disk is never modified); cleared whenever a merge changes
    // so it always reflects the current session buffer, and discarded entirely at
    // exit, so a new run re-reads the clean tilesets and re-evaluates from scratch.
    mutable QHash<QString, QPixmap> effectiveSheets_;
    // (tsx, tileId) -> index into tiles_, to resolve a click on a tileset view.
    QHash<QPair<QString, int>, int> tile_index_;

    // Undo support: merge_log_ records every tile whose merged_into was set (in
    // order); each user decision pushes an UndoStep capturing the pre-decision
    // state so BACK can revert it and re-show that prompt.
    std::vector<int> merge_log_;
    struct UndoStep
    {
        int i;
        int j;
        std::size_t mergeLogLen;   // merge_log_ length before this decision
        int identical;
        int similar;
        int skips;
        quint64 skipHashAdded;     // hash this decision added to skipped_sources_, or 0
    };
    std::vector<UndoStep> undo_;

    // Background map-writer: serialized map bytes are queued here and written by
    // writer_thread_; the queue is drained and the thread joined before exit.
    struct WriteJob
    {
        QString fileName;
        QByteArray data;
    };
    std::thread writer_thread_;
    std::mutex write_mutex_;
    std::condition_variable write_cv_;
    std::queue<WriteJob> write_queue_;
    bool write_done_;

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
    int cells_cleared_;        // visual cells replaced by transparent space

    QTimer *step_timer_;
    QLabel *status_label_;
    QWidget *decision_widget_;
    QLabel *prompt_label_;
    QLabel *preview_a_;
    QLabel *preview_b_;
    QLabel *info_a_;
    QLabel *info_b_;
    QPushButton *skip_button_;
    QPushButton *back_button_;
    QPushButton *save_button_;
    QPushButton *keep_left_button_;
    QPushButton *keep_right_button_;
    QScrollArea *scroll_a_;
    QScrollArea *scroll_b_;
    QLabel *tileset_a_;
    QLabel *tileset_b_;
    QTimer *anim_timer_;
    bool anim_on_;
    int pick_left_;             // manually-picked source tile (left sheet), or -1
    int pick_right_;             // manually-picked dest tile (right sheet), or -1
    bool dirty_;               // unsaved changes since the last write
};

#endif // TILEDEDUPLICATOR_HPP
