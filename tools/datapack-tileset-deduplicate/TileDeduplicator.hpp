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
#include <QFile>
#include <QSet>

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
class QHBoxLayout;
class QCheckBox;

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
    explicit TileDeduplicator(const QString &datapackPath, bool batch = false, bool checkAll = false, bool resetSkips = false,
                              bool stat = false, const QString &migrateFrom = QString(), const QString &migrateTo = QString(),
                              const QString &removeTsx = QString(), const QString &moveFrom = QString(),
                              const QString &moveTo = QString(), bool moveImage = false, bool mergeUsed = false,
                              const QString &mergeOut = QString(), const QStringList &mergeKeep = QStringList(),
                              QWidget *parent = nullptr);
    ~TileDeduplicator() override;

private slots:
    void loadAndStart();       // first event-loop tick: load tilesets, begin
    void onStep();             // process a bounded batch of comparisons
    void onSkipClicked();      // user kept every tile of the SIMILAR cluster
    void onBackClicked();      // revert the current cluster's decisions, re-show prompt
    void onAnimTick();         // blink the tile markers on the tileset views
    void onKeepColumnClicked();  // "KEEP THIS TILE": keep one column, drop the rest

protected:
    // A click on a column preview keeps that tile (the other columns are dropped);
    // resizing rescales the crisp previews so they grow when maximized; closing the
    // window flushes the current cluster's pending changes to disk.
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    int previewZoom() const;   // current magnification from the preview size
    void pushUndo();           // record the pre-decision state for BACK
    std::string tileKey(int idx) const;  // "<canonical tsx>#<id>" (skip/remap key)
    enum CompareLevel : std::uint8_t { Identical, Similar, Different };

    // Cluster decision view: a SIMILAR prompt shows one column per tile similar to
    // the source — all of them, in a horizontally-scrollable row (no cap).
    // buildCluster() gathers them; showCluster() (re)builds and paints the columns;
    // keepColumn() treats a column as the GROUP MASTER (kept), merges every other
    // TICKED column into it, and leaves the rest pending (the view refreshes with
    // them until fewer than 2 remain, then finishCluster() resumes scanning);
    // selectInColumn() re-picks a column's tile from its sheet.
    void buildCluster(int source);
    void showCluster();
    void rebuildColumns();
    void clearColumns();
    void renderColumn(int colIndex, bool highlightOn);
    void keepColumn(int colIndex);
    void finishCluster();
    void selectInColumn(int colIndex, const QPoint &pos);

    // --migrate-from/--migrate-to: rewrite every map to use the migrate-to tileset
    // instead of the migrate-from one (same local tile ids), then delete the
    // migrate-from .tsx and its image. Runs instead of the dedup pass.
    void runMigrate();
    // --stat: print each .tsx under the path with how many tile cells reference it
    // (counting repeats) and in how many maps. Runs instead of the dedup pass;
    // statCountLayers() tallies the cells of one map's layers (recursing into groups).
    void runStat();
    void statCountLayers(const QList<Tiled::Layer *> &layers,
                         const QHash<const Tiled::Tileset *, QString> &canonByTs,
                         QHash<QString, qint64> &tilesByTsx, QSet<QString> &seen);
    // --remove <.tsx>: clear every cell that uses the tileset (and drop its <tileset>
    // entry) from all maps, then delete the .tsx and its image. Runs instead of dedup.
    void runRemove();
    void clearTilesetCells(const QList<Tiled::Layer *> &layers, const Tiled::Tileset *ts, int &cleared);
    // --move-from <.tsx> --move-to <path/.tsx>: physically relocate the .tsx to a new
    // path (same tiles), then rewrite every .tmx under the path so its <tileset source>
    // uses the new RELATIVE path. The image is NOT moved; the relocated .tsx's <image
    // source> is rewritten relative to its new location. Runs instead of dedup.
    void runMove();
    // Dangling-safe repoint: in a .tmx's raw TEXT, change the <tileset> entry whose
    // source resolves to `fromCanon` so it points at `toAbs` (relative to the map dir),
    // leaving every other <tileset> (incl. an intentional unresolved missing.tsx) and all
    // encoded layer data byte-for-byte untouched. Used by --migrate/--move for maps that
    // libtiled can't re-serialize without corruption (they carry an unresolved tileset).
    // Returns true if a matching entry was found and rewritten.
    bool repointTilesetSourceInText(const QString &tmxPath, const QString &fromCanon, const QString &toAbs);
    // --merge-used: across every .tmx under the path, gather the tiles actually used from
    // each RESOLVABLE tileset (except those named in --merge-keep and any unresolved one),
    // pack the unique ones (identical tiles collapsed) into ONE new tileset (.tsx + .png at
    // --merge-out), then rewrite each map's gids/object-gids to that single tileset, drop
    // the merged-away <tileset> entries and append the new one. Kept and unresolved tilesets
    // (e.g. an intentional missing.tsx) and all other layer data are preserved. Runs instead
    // of dedup. Works at the gid/base64/zstd level so dangling maps are handled too.
    void runMergeUsed();

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
    void showPrompt(int source);   // gather the cluster of tiles similar to source and prompt
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

    // Append-only audit log (<datapack>/tile-dedup.log). One line per committed merge
    // (AUTO/USER, loser -> keeper, written after the maps/png are on disk) and one
    // per skipped tile (SKIP, written immediately). ensureLogOpen() opens it lazily
    // and writes the per-run header; replaySkips() reloads the SKIP lines at startup
    // so SKIP decisions survive a restart (merges resume via the blanked png).
    bool ensureLogOpen();
    void logMerge(int loser, int keeper, bool automatic);
    void logSkip(int idx);
    void replaySkips();
    void purgeSkipLines();  // --reset-skips: drop all SKIP lines from the log

    // Commit every merge made since the last commit straight to disk: rewrite the
    // affected .tmx maps to the keeper, blank each merged loser tile to transparent
    // in its tileset .png (so a restart re-reads it as empty and never re-detects
    // it), then append the merges to the log. Called when a cluster is finished and
    // when the window is closed — there is no manual SAVE.
    void flushChanges();
    // Build (once, lazily) the canonical-tsx -> [.tmx files] index used by
    // flushChanges() to rewrite only the maps that reference a merged tile.
    void buildMapIndex();

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
    bool reset_skips_;         // --reset-skips: forget all previously SKIP-ped tiles
    bool stat_;                // --stat: list .tsx with their .tmx usage count, then quit
    QString migrate_from_;     // --migrate-from <.tsx>: tileset to replace then delete
    QString migrate_to_;       // --migrate-to <.tsx>: tileset to migrate every map to
    QString remove_tsx_;       // --remove <.tsx>: tileset to delete + clear from all maps
    QString move_from_;        // --move-from <.tsx>: tileset to relocate
    QString move_to_;          // --move-to <path/.tsx>: new path for the relocated tileset
    bool move_image_;          // --move-image: relocate the .png alongside the moved .tsx
    bool merge_used_;          // --merge-used: pack used tiles of all maps into one tileset
    QString merge_out_;        // --merge-out <path/.tsx>: the single output tileset
    QStringList merge_keep_;   // --merge-keep: tileset basenames to leave as separate refs
    std::vector<TileEntry> tiles_;
    int i_;
    int j_;
    // "<canonical tsx>#<id>" of tiles the user chose to leave alone via SKIP. They
    // are excluded from every later prompt (as source and as candidate) and persisted
    // in the log, so SKIP is remembered across restarts.
    std::unordered_set<std::string> skipped_tiles_;
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

    // Every merge made this run, in order: the loser tile, the keeper it points at,
    // and whether it was automatic. merge_log_committed_ is how many of these have
    // already been written to disk (maps + png) and logged by flushChanges(); the
    // rest are pending. BACK only ever touches the pending tail (the current,
    // not-yet-committed cluster).
    struct MergeRec
    {
        int loser;
        int keeper;
        bool automatic;
    };
    std::vector<MergeRec> merge_log_;
    std::size_t merge_log_committed_;
    // Undo support: each user decision pushes an UndoStep capturing the pre-decision
    // state so BACK can revert it and re-show that prompt (within the current cluster
    // only — undo_ is cleared once the cluster is committed to disk).
    struct UndoStep
    {
        int i;
        int j;
        std::size_t mergeLogLen;   // merge_log_ length before this decision
        int identical;
        int similar;
        int skips;
        std::vector<int> cluster;  // the pending set shown before this decision (for BACK)
    };
    std::vector<UndoStep> undo_;

    // canonical .tsx -> .tmx files that reference it (built once by buildMapIndex())
    // and the canonical .tsx -> tileset .png path, so flushChanges() can rewrite only
    // the affected maps and blank the merged tiles in the right image.
    QHash<QString, QStringList> maps_by_tsx_;
    QHash<QString, QString> png_by_tsx_;
    bool map_index_built_;
    // canonical .tsx whose sheet became fully transparent (all tiles merged away):
    // its <tileset> entry is removed from every map, and the .tsx/.png are deleted.
    std::unordered_set<std::string> dead_tsx_;

    // Append-only merge audit log, opened lazily on the first merge and kept open
    // for the whole run (closed — and thereby flushed — in the destructor).
    QFile log_file_;
    bool log_open_tried_;      // we already attempted to open log_file_ this run

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
    QPushButton *skip_button_;
    QPushButton *back_button_;
    QTimer *anim_timer_;
    bool anim_on_;

    // One on-screen column of the cluster decision view: a "group" tick-box, a
    // magnified preview, an info label, the tile's whole tileset (scrollable, with a
    // blinking marker) and a "KEEP THIS (MASTER)" button. cur is the tile this column
    // currently stands for (the cluster member by default, re-pickable from its sheet).
    struct Column
    {
        QWidget *box;
        QCheckBox *check;
        QLabel *preview;
        QLabel *info;
        QScrollArea *scroll;
        QLabel *tileset;
        QPushButton *keep;
        int cur;
    };
    QWidget *columns_container_;     // holds the columns row
    QHBoxLayout *columns_layout_;    // one QWidget box per column, rebuilt each prompt
    QScrollArea *columns_scroll_;    // horizontal scroll wrapper around the columns row
    std::vector<Column> columns_;
    std::vector<int> cluster_;       // tiles currently pending in the prompt (>=2 to show)
};

#endif // TILEDEDUPLICATOR_HPP
