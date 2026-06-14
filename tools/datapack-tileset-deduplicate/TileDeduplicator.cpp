#include "TileDeduplicator.hpp"

#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QTimer>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QColor>
#include <QEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QSizePolicy>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QBuffer>
#include <QFile>
#include <QDateTime>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QDebug>

#include <map.h>
#include <layer.h>
#include <grouplayer.h>
#include <tilelayer.h>
#include <objectgroup.h>
#include <mapobject.h>
#include <tileset.h>
#include <tile.h>
#include <mapreader.h>
#include <mapwriter.h>
#include <compression.h>

#include <cstring>
#include <algorithm>
#include <iostream>

// All tiles in this datapack are 16x16; the spec fixes the size.
static const int kTileW = 16;
static const int kTileH = 16;
static const int kTilePx = kTileW * kTileH;               // 256

// SIMILAR threshold: at least 90% of the RELEVANT pixels must match. A pixel is
// relevant when at least one of the two tiles is non-transparent there — i.e. it
// belongs to the visible footprint of either tile. Pixels transparent on BOTH
// sides are ignored, so a mostly-transparent tile is judged on its few visible
// pixels, not diluted by the large matching empty background (two small sprites
// drawn at different places, or differing in most of their visible pixels, then
// come out DIFFERENT instead of falsely SIMILAR).
// kMaxMismatch is the hard early-exit cap: even when the whole 16x16 tile is
// relevant, a 90% match allows at most floor(0.1*256)=25 mismatches, so once more
// than that mismatch the pair can never be SIMILAR and comparePixels() bails out.
static const int kSimilarPercent = 90;
static const int kMaxMismatch = kTilePx * (100 - kSimilarPercent) / 100; // 25

// A pixel whose alpha is at/below this is treated as fully transparent: its
// RGB is not rendered, so two such pixels match regardless of their stored RGB
// (PNGs often keep garbage RGB under transparent areas). ~5% of 255.
static const int kAlphaTransparent = 13;

static const int kZoom = 16;          // preview magnification (16x16 -> 256x256)
static const int kStepBudget = 200000; // comparisons handled per event-loop tick

// Per-channel "within +-15%" test: the difference must be within 15% of the
// larger value, and a 0 requires an exact 0 on the other side. Integer form of
// |x-y| <= 0.15*max(x,y).
static inline bool within15(int x, int y)
{
    const int d = (x > y) ? (x - y) : (y - x);
    const int m = (x > y) ? x : y;
    return d * 100 <= m * 15;
}

// Parse the frame count from a tile's custom "animation" property value, e.g.
// "400ms;2frames" -> 2, "100ms;6frames" -> 6. Such a tile is the FIRST frame of an
// animation occupying it plus the next (frames-1) tile ids; all of them must be
// ignored. The count is the leading number of the ";"-separated token that ends
// in "frames". Returns 0 when no such token is found.
// True when every pixel of the image is (near-)transparent — i.e. the whole sheet is
// empty, so the tileset has no visible content left and can be deleted.
static bool isFullyTransparent(const QImage &img)
{
    int y = 0;
    while(y < img.height())
    {
        const QRgb *line = reinterpret_cast<const QRgb *>(img.constScanLine(y));
        int x = 0;
        while(x < img.width())
        {
            if(qAlpha(line[x]) > kAlphaTransparent)
                return false;
            x++;
        }
        y++;
    }
    return true;
}

static int animationFrameCount(const QString &value)
{
    const QStringList parts = value.split(QLatin1Char(';'));
    int p = 0;
    while(p < parts.size())
    {
        const QString tok = parts.at(p).trimmed();
        if(tok.contains(QLatin1String("frame"), Qt::CaseInsensitive))
        {
            QString num;
            int c = 0;
            while(c < tok.size() && tok.at(c).isDigit())
            {
                num.append(tok.at(c));
                c++;
            }
            if(!num.isEmpty())
                return num.toInt();
        }
        p++;
    }
    return 0;
}

TileDeduplicator::TileDeduplicator(const QString &datapackPath, bool batch, bool checkAll, bool resetSkips,
                                   bool stat, const QString &migrateFrom, const QString &migrateTo,
                                   const QString &removeTsx, const QString &moveFrom, const QString &moveTo,
                                   bool moveImage, bool mergeUsed, const QString &mergeOut, const QStringList &mergeKeep,
                                   QWidget *parent) :
    QWidget(parent),
    datapack_path_(datapackPath),
    batch_(batch),
    check_all_(checkAll),
    reset_skips_(resetSkips),
    stat_(stat),
    migrate_from_(migrateFrom),
    migrate_to_(migrateTo),
    remove_tsx_(removeTsx),
    move_from_(moveFrom),
    move_to_(moveTo),
    move_image_(moveImage),
    merge_used_(mergeUsed),
    merge_out_(mergeOut),
    merge_keep_(mergeKeep),
    i_(0),
    j_(1),
    merge_log_committed_(0),
    map_index_built_(false),
    log_open_tried_(false),
    write_done_(false),
    map_changed_(false),
    identical_merges_(0),
    similar_merges_(0),
    skips_(0),
    maps_rewritten_(0),
    maps_skipped_(0),
    cells_cleared_(0),
    step_timer_(new QTimer(this)),
    status_label_(new QLabel(this)),
    decision_widget_(new QWidget(this)),
    prompt_label_(new QLabel(this)),
    skip_button_(new QPushButton(tr("SKIP"), decision_widget_)),
    back_button_(new QPushButton(tr("BACK"), decision_widget_)),
    anim_timer_(new QTimer(this)),
    anim_on_(true),
    columns_container_(new QWidget(decision_widget_)),
    columns_layout_(nullptr),
    columns_scroll_(nullptr)
{
    setWindowTitle(tr("CatchChallenger - Tileset tile deduplicator"));

    QVBoxLayout *root = new QVBoxLayout(this);
    status_label_->setText(tr("Loading tilesets..."));
    root->addWidget(status_label_);

    prompt_label_->setText(tr("SIMILAR tiles \342\200\224 one column per similar tile (scroll "
                              "horizontally for more). Mark a GROUP: tick the tiles that are the "
                              "same (or click their big preview), then click \342\200\234KEEP THIS "
                              "TILE\342\200\235 under the one to keep \342\200\224 the other ticked tiles "
                              "merge into it and all maps repoint to it. Only the merged tiles drop "
                              "out; the kept tile and the untouched ones STAY in the list so you can "
                              "keep grouping more tiles into the kept one. Click a tile inside a "
                              "column\342\200\231s tileset to re-pick which tile that column stands for. "
                              "SKIP keeps the rest and moves on."));
    prompt_label_->setWordWrap(true);

    // The columns row is rebuilt for each prompt (the column count varies) in
    // rebuildColumns(); it lives inside a horizontal scroll area so an unbounded
    // number of similar tiles can be shown.
    columns_layout_ = new QHBoxLayout(columns_container_);
    columns_scroll_ = new QScrollArea(decision_widget_);
    columns_scroll_->setWidget(columns_container_);
    columns_scroll_->setWidgetResizable(true);
    columns_scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    columns_scroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    columns_scroll_->setMinimumHeight(620);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addWidget(back_button_);
    buttons->addWidget(skip_button_);

    QVBoxLayout *decisionLayout = new QVBoxLayout(decision_widget_);
    decisionLayout->addWidget(prompt_label_);
    decisionLayout->addWidget(columns_scroll_);
    decisionLayout->addLayout(buttons);
    root->addWidget(decision_widget_);
    decision_widget_->hide();

    connect(skip_button_, &QPushButton::clicked, this, &TileDeduplicator::onSkipClicked);
    connect(back_button_, &QPushButton::clicked, this, &TileDeduplicator::onBackClicked);

    anim_timer_->setInterval(450);
    connect(anim_timer_, &QTimer::timeout, this, &TileDeduplicator::onAnimTick);

    step_timer_->setInterval(0);
    connect(step_timer_, &QTimer::timeout, this, &TileDeduplicator::onStep);

    // Load + start after the window is shown, so "Loading tilesets..." is
    // visible during the (blocking) tileset read.
    QTimer::singleShot(0, this, &TileDeduplicator::loadAndStart);
}

TileDeduplicator::~TileDeduplicator()
{
    // Close the append-only merge log; this flushes the buffered lines (we never
    // flush explicitly during the run, as requested — append all, never flush).
    if(log_file_.isOpen())
        log_file_.close();
}

void TileDeduplicator::loadAndStart()
{
    if(stat_)
    {
        runStat();             // print tileset usage stats, then quit
        close();
        QCoreApplication::quit();
        return;
    }
    if(!migrate_from_.isEmpty())
    {
        runMigrate();          // one-shot tileset migration, then quit
        close();
        QCoreApplication::quit();
        return;
    }
    if(!remove_tsx_.isEmpty())
    {
        runRemove();           // delete a tileset and clear it from all maps, then quit
        close();
        QCoreApplication::quit();
        return;
    }
    if(!move_from_.isEmpty())
    {
        runMove();             // relocate a tileset and fix every map's relative path, then quit
        close();
        QCoreApplication::quit();
        return;
    }
    if(merge_used_)
    {
        runMergeUsed();        // pack used tiles into one tileset and rewrite maps, then quit
        close();
        QCoreApplication::quit();
        return;
    }
    if(!loadTilesets())
    {
        status_label_->setText(tr("No 16x16 tileset found under: %1").arg(datapack_path_));
        return;
    }
    if(reset_skips_)
        purgeSkipLines();   // --reset-skips: forget every previously skipped tile
    else
        replaySkips();      // restore SKIP decisions (merges resume via the blanked png)
    if(tiles_.size() < 2)
    {
        // Nothing to compare: close straight away.
        finish();
        return;
    }
    i_ = 0;
    j_ = 1;
    updateProgress();
    step_timer_->start();
}

// Read every .tsx in the datapack, split each into 16x16 tiles and keep the
// non-empty ones. Animated tiles are skipped entirely (merging would drop their
// frames). Tiles carrying custom properties ARE loaded — so they show in the
// views and can be merged by hand — but are flagged has_properties so the O(n^2)
// pass never auto-merges them (that would silently drop the property metadata).
bool TileDeduplicator::loadTilesets()
{
    QStringList tsxFiles;
    {
        QDirIterator it(datapack_path_, QStringList() << QStringLiteral("*.tsx"),
                        QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext())
            tsxFiles.append(it.next());
    }
    tsxFiles.sort();

    int fi = 0;
    while(fi < tsxFiles.size())
    {
        const QString &tsxPath = tsxFiles.at(fi);
        Tiled::MapReader reader;
        Tiled::SharedTileset tileset = reader.readTileset(tsxPath);
        // Standalone readTileset() only records the image reference; we have to
        // load the pixmap ourselves (the map-reading path does this too). This
        // also splits the sheet into per-tile imageRects.
        if(!tileset.isNull())
            tileset->loadImage();
        if(tileset.isNull())
            qWarning() << "Unable to read tileset" << tsxPath << ":" << reader.errorString();
        else if(tileset->tileWidth() != kTileW || tileset->tileHeight() != kTileH)
            qWarning() << "Skipping non-16x16 tileset" << tsxPath;
        else
        {
            const QString canonTsx = QFileInfo(tsxPath).canonicalFilePath();
            const QImage sheet = tileset->image().toImage().convertToFormat(QImage::Format_ARGB32);
            // Keep the whole sheet (1x) + its column count so a prompt can show
            // the tile inside its tileset with a blinking marker.
            if(!sheet.isNull() && sheet.width() >= kTileW)
            {
                sheets_.insert(canonTsx, QPixmap::fromImage(sheet));
                sheetCols_.insert(canonTsx, sheet.width() / kTileW);
                // Remember the on-disk .png so a merged tile can be blanked there.
                png_by_tsx_.insert(canonTsx, tileset->imageSource().toLocalFile());
            }

            const QList<Tiled::Tile *> &tiles = tileset->tiles();

            // Pre-pass: a custom "animation" property (e.g. "100ms;6frames") names
            // the first frame of an animation that spans this tile plus the next
            // (frames-1) tile ids. Collect every such frame id so the main pass
            // ignores them all — like libtiled-native animated tiles, they must be
            // left completely untouched (never compared, merged or cleared).
            std::unordered_set<int> animationIds;
            {
                int ai = 0;
                while(ai < tiles.size())
                {
                    Tiled::Tile *atile = tiles.at(ai);
                    const QVariant av = atile->properties().value(QStringLiteral("animation"));
                    if(av.isValid())
                    {
                        int frames = animationFrameCount(av.toString());
                        if(frames < 1)
                            frames = 1;   // unparseable value: at least ignore the tile itself
                        int f = 0;
                        while(f < frames)
                        {
                            animationIds.insert(atile->id() + f);
                            f++;
                        }
                    }
                    ai++;
                }
            }

            int ti = 0;
            while(ti < tiles.size())
            {
                Tiled::Tile *tile = tiles.at(ti);
                if(tile->isAnimated() || animationIds.count(tile->id()) != 0)
                {
                    // keep animated tiles untouched (merging drops their frames);
                    // tiles covered by a custom "animation" property span are
                    // ignored the same way
                }
                else
                {
                    // A custom property is gameplay metadata: the tile is still
                    // loaded (visible, hand-mergeable) but flagged so it is never
                    // auto-merged.
                    const bool hasProps = !tile->properties().isEmpty();
                    // The tile's pixels live in a source pixmap indexed by
                    // imageRect(). For single-image tilesets that source is the
                    // shared sheet (tile->image() returns the whole sheet, so we
                    // must crop by imageRect); for collection tilesets the sheet
                    // is null and the tile carries its own image (imageRect then
                    // spans the whole image).
                    QImage src = sheet;
                    if(src.isNull() && !tile->image().isNull())
                        src = tile->image().toImage().convertToFormat(QImage::Format_ARGB32);
                    const QRect rect = tile->imageRect();
                    QImage tileImage;
                    if(!src.isNull() && rect.isValid())
                        tileImage = src.copy(rect);

                    if(tileImage.width() == kTileW && tileImage.height() == kTileH)
                    {
                        TileEntry entry;
                        entry.tsx = canonTsx;
                        entry.id = tile->id();
                        entry.px.resize(kTilePx);
                        entry.alpha_sum = 0;
                        int y = 0;
                        while(y < kTileH)
                        {
                            const QRgb *line = reinterpret_cast<const QRgb *>(tileImage.constScanLine(y));
                            int x = 0;
                            while(x < kTileW)
                            {
                                const QRgb p = line[x];
                                entry.px[y * kTileW + x] = p;
                                entry.alpha_sum += qAlpha(p);
                                x++;
                            }
                            y++;
                        }
                        // Skip fully transparent tiles: they are empty cells,
                        // merging them changes nothing but would explode the
                        // O(n^2) comparison with thousands of equal blanks.
                        if(entry.alpha_sum > 0)
                        {
                            // FNV-1a over the raw pixel bytes.
                            quint64 h = 1469598103934665603ULL;
                            const unsigned char *bytes = reinterpret_cast<const unsigned char *>(entry.px.data());
                            const int n = kTilePx * static_cast<int>(sizeof(QRgb));
                            int bi = 0;
                            while(bi < n)
                            {
                                h ^= bytes[bi];
                                h *= 1099511628211ULL;
                                bi++;
                            }
                            entry.hash = h;
                            entry.merged_into = -1;
                            entry.has_properties = hasProps;
                            tile_index_.insert(qMakePair(canonTsx, entry.id), static_cast<int>(tiles_.size()));
                            tiles_.push_back(std::move(entry));
                        }
                        else
                        {
                            // Fully transparent: an empty tile. Keep it out of the
                            // O(n^2) compare (thousands of equal blanks), but record
                            // it so every visual tile-layer cell pointing at it is
                            // replaced by transparent space on rewrite. A transparent
                            // tile carrying a custom property is a marker (e.g.
                            // invisible.tsx) — never auto-clear it.
                            if(!hasProps)
                                transparent_tiles_.insert((canonTsx + QStringLiteral("#") + QString::number(entry.id)).toStdString());
                        }
                    }
                }
                ti++;
            }
        }
        fi++;
    }

    qDebug() << "Loaded" << static_cast<int>(tiles_.size()) << "non-empty 16x16 tiles from"
             << tsxFiles.size() << "tileset(s)";
    return !tsxFiles.isEmpty();
}

bool TileDeduplicator::isPresenceLayer(const QString &name)
{
    // Layers where a non-empty cell means block / encounter / surf / ledge
    // regardless of the tile's pixels; clearing a transparent cell here would
    // silently delete that gameplay marker.
    return name == QLatin1String("Collisions")
        || name == QLatin1String("Grass")
        || name == QLatin1String("OnGrass")
        || name == QLatin1String("Water")
        || name == QLatin1String("LedgesDown")
        || name == QLatin1String("LedgesLeft")
        || name == QLatin1String("LedgesRight");
}

TileDeduplicator::CompareLevel TileDeduplicator::comparePixels(const TileEntry &a, const TileEntry &b)
{
    // Skip detection entirely for a fully-transparent tile: it has no visible
    // content, so it is neither IDENTICAL nor SIMILAR to anything. (Such tiles are
    // already kept out of tiles_ at load; this guards any other path.)
    if(a.alpha_sum == 0 || b.alpha_sum == 0)
        return Different;

    // Exact-byte fast path (guard against the rare hash collision).
    if(a.hash == b.hash &&
       std::memcmp(a.px.data(), b.px.data(), kTilePx * sizeof(QRgb)) == 0)
        return Identical;

    int mismatch = 0;
    int relevant = 0;          // pixels visible on at least one side (the compared area)
    bool allMatch = true;
    int k = 0;
    while(k < kTilePx)
    {
        const QRgb pa = a.px[k];
        const QRgb pb = b.px[k];
        const int aa = qAlpha(pa);
        const int ab = qAlpha(pb);
        const bool bothTransparent = (aa <= kAlphaTransparent && ab <= kAlphaTransparent);
        if(!bothTransparent)
            relevant++;        // part of the visible footprint of a and/or b

        bool pixelMatch;
        if(!within15(aa, ab))
            pixelMatch = false;
        else if(bothTransparent)
            pixelMatch = true; // both transparent: RGB is not rendered
        else
            pixelMatch = within15(qRed(pa), qRed(pb)) &&
                         within15(qGreen(pa), qGreen(pb)) &&
                         within15(qBlue(pa), qBlue(pb));

        if(!pixelMatch)
        {
            allMatch = false;
            mismatch++;
            if(mismatch > kMaxMismatch)
                return Different;
        }
        k++;
    }
    if(allMatch)
        return Identical;
    // SIMILAR only when the mismatches are within (100-kSimilarPercent)% of the
    // RELEVANT area, not of the whole tile: a mostly-transparent pair whose few
    // visible pixels differ (or sit at different places) is DIFFERENT, not similar.
    if(relevant > 0 && mismatch * 100 <= relevant * (100 - kSimilarPercent))
        return Similar;
    return Different;
}

int TileDeduplicator::findRoot(int idx) const
{
    while(tiles_[idx].merged_into != -1)
        idx = tiles_[idx].merged_into;
    return idx;
}

bool TileDeduplicator::identityLess(const TileEntry &a, const TileEntry &b) const
{
    const int c = QString::compare(a.tsx, b.tsx);
    if(c != 0)
        return c < 0;
    return a.id < b.id;
}

// Keep the less-transparent tile (higher alpha sum); on a tie keep the one
// first in alphabetical order. The loser points at the keeper. *sourceLost is
// set when tile a (the outer/source tile) is the one merged away.
void TileDeduplicator::mergePair(int a, int b, bool *sourceLost)
{
    const TileEntry &ta = tiles_[a];
    const TileEntry &tb = tiles_[b];
    int keeper;
    int loser;
    if(ta.alpha_sum > tb.alpha_sum)
    {
        keeper = a;
        loser = b;
    }
    else if(tb.alpha_sum > ta.alpha_sum)
    {
        keeper = b;
        loser = a;
    }
    else
    {
        if(identityLess(ta, tb))
        {
            keeper = a;
            loser = b;
        }
        else
        {
            keeper = b;
            loser = a;
        }
    }
    tiles_[loser].merged_into = keeper;
    MergeRec rec;
    rec.loser = loser;
    rec.keeper = keeper;
    rec.automatic = true;
    merge_log_.push_back(rec);   // committed to disk later by flushChanges()
    effectiveSheets_.clear();  // merge changed: rebuild the "with changes" sheets
    if(sourceLost != nullptr)
        *sourceLost = (loser == a);
}

// Append one line to the merge audit log the instant a merge is committed. The
// log (<datapack>/tile-dedup.log) is opened lazily here in Append mode — never
// truncated — and kept open for the run; a per-run header precedes the first
// line. automatic=true marks an IDENTICAL auto-merge, false a user decision.
// Open the append-only log lazily, writing the per-run header once. Returns whether
// the log is usable.
bool TileDeduplicator::ensureLogOpen()
{
    if(!log_open_tried_)
    {
        log_open_tried_ = true;
        log_file_.setFileName(QDir(datapack_path_).filePath(QStringLiteral("tile-dedup.log")));
        if(!log_file_.open(QFile::Append | QFile::Text))
            qWarning() << "Unable to open log" << log_file_.fileName() << ":" << log_file_.errorString();
        else
            log_file_.write(QStringLiteral("# === run %1 ===\n")
                            .arg(QDateTime::currentDateTime().toString(Qt::ISODate)).toUtf8());
    }
    return log_file_.isOpen();
}

void TileDeduplicator::logMerge(int loser, int keeper, bool automatic)
{
    if(!ensureLogOpen())
        return;
    const TileEntry &lo = tiles_[loser];
    const TileEntry &ke = tiles_[keeper];
    log_file_.write(QStringLiteral("%1  %2  %3#%4  ->  %5#%6\n")
                    .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
                    .arg(automatic ? QStringLiteral("AUTO") : QStringLiteral("USER"))
                    .arg(lo.tsx).arg(lo.id)
                    .arg(ke.tsx).arg(ke.id).toUtf8());
}

// Record a skipped tile so it persists across restarts.
void TileDeduplicator::logSkip(int idx)
{
    if(!ensureLogOpen())
        return;
    log_file_.write(QStringLiteral("%1  SKIP  %2#%3\n")
                    .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
                    .arg(tiles_[idx].tsx).arg(tiles_[idx].id).toUtf8());
}

std::string TileDeduplicator::tileKey(int idx) const
{
    return (tiles_[idx].tsx + QStringLiteral("#") + QString::number(tiles_[idx].id)).toStdString();
}

// Reload SKIP decisions from a previous run: scan the log for "… SKIP <tsx>#<id>"
// lines and re-populate skipped_tiles_, so the same tiles are not offered again.
// (Merges are not replayed here — their loser tile is already blanked in the png and
// so excluded at load.)
void TileDeduplicator::replaySkips()
{
    QFile f(QDir(datapack_path_).filePath(QStringLiteral("tile-dedup.log")));
    if(!f.open(QFile::ReadOnly | QFile::Text))
        return;
    const QString text = QString::fromUtf8(f.readAll());
    f.close();
    const QStringList lines = text.split(QLatin1Char('\n'));
    const QString sep = QStringLiteral("  SKIP  ");
    int i = 0;
    while(i < lines.size())
    {
        const QString line = lines.at(i);
        i++;
        const int sk = line.indexOf(sep);
        if(sk < 0)
            continue;
        const QString key = line.mid(sk + sep.size()).trimmed();
        if(!key.isEmpty())
            skipped_tiles_.insert(key.toStdString());
    }
}

// --reset-skips: drop every SKIP line from the log (keeping the merge audit lines),
// so no tile is remembered as skipped. skipped_tiles_ is left empty for this run.
void TileDeduplicator::purgeSkipLines()
{
    const QString path = QDir(datapack_path_).filePath(QStringLiteral("tile-dedup.log"));
    QFile f(path);
    if(!f.open(QFile::ReadOnly | QFile::Text))
        return;            // no log yet: nothing skipped
    const QStringList lines = QString::fromUtf8(f.readAll()).split(QLatin1Char('\n'));
    f.close();
    const QString sep = QStringLiteral("  SKIP  ");
    QStringList kept;
    int removed = 0;
    int i = 0;
    while(i < lines.size())
    {
        if(lines.at(i).indexOf(sep) >= 0)
            removed++;
        else
            kept.append(lines.at(i));
        i++;
    }
    if(removed > 0 && f.open(QFile::WriteOnly | QFile::Truncate | QFile::Text))
    {
        f.write(kept.join(QLatin1Char('\n')).toUtf8());
        f.close();
    }
    qDebug() << "Reset skips: removed" << removed << "SKIP entries from" << path;
}

void TileDeduplicator::advanceSource()
{
    i_++;
    j_ = i_ + 1;
    updateProgress();
}

void TileDeduplicator::updateProgress()
{
    const int total = static_cast<int>(tiles_.size());
    int current = i_ + 1;
    if(current > total)
        current = total;
    status_label_->setText(tr("processing... %1/%2 tiles").arg(current).arg(total));
}

void TileDeduplicator::onStep()
{
    const int n = static_cast<int>(tiles_.size());
    int budget = kStepBudget;
    while(budget-- > 0)
    {
        if(i_ >= n - 1)
        {
            finish();
            return;
        }
        if(tiles_[i_].merged_into != -1)
        {
            advanceSource();
            continue;
        }
        if(j_ >= n)
        {
            advanceSource();
            continue;
        }
        if(tiles_[j_].merged_into != -1)
        {
            j_++;
            continue;
        }
        // By default only compare tiles ACROSS different tilesets (within-file
        // duplicates are usually intentional); --check-all also compares tiles
        // that live in the same .tsx.
        if(!check_all_ && tiles_[i_].tsx == tiles_[j_].tsx)
        {
            j_++;
            continue;
        }

        const CompareLevel level = comparePixels(tiles_[i_], tiles_[j_]);
        // Only act on a pair when NEITHER tile carries a custom property: auto-merge
        // it when IDENTICAL, ask the user when SIMILAR. If either tile has a custom
        // property we neither auto-merge (that would drop the property's gameplay
        // metadata) nor even prompt — just skip it. Such tiles are still loaded and
        // shown, so they can be deduplicated by hand via the tileset views.
        if(tiles_[i_].has_properties || tiles_[j_].has_properties)
            j_++;
        else if(level == Identical)
        {
            bool sourceLost = false;
            mergePair(i_, j_, &sourceLost);
            identical_merges_++;
            if(sourceLost)
                advanceSource();
            else
                j_++;
        }
        else if(level == Similar)
        {
            // Skip this pair if either tile was left alone via SKIP (remembered
            // across restarts): the source, so it is never re-asked; or this
            // candidate, so we scan past it to a non-skipped one.
            if(batch_ || skipped_tiles_.count(tileKey(i_)) != 0 || skipped_tiles_.count(tileKey(j_)) != 0)
                j_++;
            else
            {
                flushChanges();   // persist auto-merges done while scanning to here
                showPrompt(i_);   // gather every tile similar to i_ into the cluster
                return; // wait for the user's button
            }
        }
        else
            j_++;
    }
    // Budget exhausted; refresh progress and let the timer fire again.
    updateProgress();
}

QPixmap TileDeduplicator::renderPreview(const TileEntry &t, int zoom) const
{
    if(zoom < 1)
        zoom = 1;
    QImage tileImage(kTileW, kTileH, QImage::Format_ARGB32);
    int y = 0;
    while(y < kTileH)
    {
        QRgb *line = reinterpret_cast<QRgb *>(tileImage.scanLine(y));
        int x = 0;
        while(x < kTileW)
        {
            line[x] = t.px[y * kTileW + x];
            x++;
        }
        y++;
    }

    const int w = kTileW * zoom;
    const int h = kTileH * zoom;
    QPixmap canvas(w, h);
    QPainter painter(&canvas);

    // Checkerboard background so transparent parts of the tile are visible. The
    // square is HALF a zoomed pixel (kZoom/2) so each real tile pixel shows a 2x2
    // checker (4 squares per pixel) — that makes partial transparency easy to read.
    const QColor light(200, 200, 200);
    const QColor dark(120, 120, 120);
    int sq = zoom / 2;          // half a zoomed pixel => 4 checker squares per pixel
    if(sq < 1)
        sq = 1;
    int cy = 0;
    while(cy * sq < h)
    {
        int cx = 0;
        while(cx * sq < w)
        {
            const bool even = ((cx + cy) & 1) == 0;
            painter.fillRect(cx * sq, cy * sq, sq, sq, even ? light : dark);
            cx++;
        }
        cy++;
    }

    // Nearest-neighbour 16x magnification keeps the pixels crisp ("fast scaling").
    const QImage scaled = tileImage.scaled(w, h, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    painter.drawImage(0, 0, scaled);
    painter.end();
    return canvas;
}

// Gather the cluster of tiles to show: the source plus every still-active,
// property-free tile that is IDENTICAL or SIMILAR to it (respecting the
// across-tileset rule unless --check-all). The source is column 0.
void TileDeduplicator::buildCluster(int source)
{
    cluster_.clear();
    cluster_.push_back(source);
    if(tiles_[source].has_properties)
        return;            // should not happen: prompts are property-free only
    const int n = static_cast<int>(tiles_.size());
    int t = 0;
    // Gather EVERY tile similar to the source (no cap); the view scrolls horizontally.
    while(t < n)
    {
        if(t != source && tiles_[t].merged_into == -1 && !tiles_[t].has_properties
           && skipped_tiles_.count(tileKey(t)) == 0)   // never re-offer a skipped tile
        {
            if(check_all_ || tiles_[t].tsx != tiles_[source].tsx)
            {
                const CompareLevel lvl = comparePixels(tiles_[source], tiles_[t]);
                if(lvl == Identical || lvl == Similar)
                    cluster_.push_back(t);
            }
        }
        t++;
    }
}

// Delete the previous prompt's column widgets (deleting each box deletes its
// children and removes it from the layout) and forget them.
void TileDeduplicator::clearColumns()
{
    int c = 0;
    while(c < static_cast<int>(columns_.size()))
    {
        delete columns_[c].box;
        c++;
    }
    columns_.clear();
}

// (Re)create one column widget per pending tile: a "group" tick-box, a magnified
// preview (click = toggle the tick-box), an info label, the tile's tileset (click =
// re-pick this column's tile) and a "KEEP THIS (MASTER)" button tagged with its
// column index.
void TileDeduplicator::rebuildColumns()
{
    clearColumns();
    int i = 0;
    while(i < static_cast<int>(cluster_.size()))
    {
        Column col;
        col.cur = cluster_[i];
        col.box = new QWidget(columns_container_);
        col.box->setMinimumWidth(330);   // fixed-ish width so the row scrolls when full
        QVBoxLayout *v = new QVBoxLayout(col.box);

        // Ticked by default (the common case is "all of these are the same"); untick
        // the ones that are NOT a duplicate before clicking KEEP THIS TILE.
        col.check = new QCheckBox(tr("group"), col.box);
        col.check->setChecked(true);

        col.preview = new QLabel(col.box);
        col.preview->setMinimumSize(kTileW * kZoom, kTileH * kZoom);
        col.preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        col.preview->setAlignment(Qt::AlignCenter);
        col.preview->setCursor(Qt::PointingHandCursor);
        col.preview->installEventFilter(this);

        col.info = new QLabel(col.box);

        col.tileset = new QLabel();
        col.scroll = new QScrollArea(col.box);
        col.scroll->setWidget(col.tileset);
        col.scroll->setMinimumSize(300, 260);
        col.scroll->setAlignment(Qt::AlignCenter);
        col.tileset->installEventFilter(this);
        col.tileset->setCursor(Qt::PointingHandCursor);

        col.keep = new QPushButton(tr("KEEP THIS TILE"), col.box);
        col.keep->setProperty("col", i);   // read back in onKeepColumnClicked()
        connect(col.keep, &QPushButton::clicked, this, &TileDeduplicator::onKeepColumnClicked);

        v->addWidget(col.check);
        v->addWidget(col.preview);
        v->addWidget(col.info);
        v->addWidget(col.scroll);
        v->addWidget(col.keep);
        columns_layout_->addWidget(col.box);
        columns_.push_back(col);
        i++;
    }
}

// Paint one column: its preview, info line and tileset view (blinking marker).
void TileDeduplicator::renderColumn(int colIndex, bool highlightOn)
{
    Column &col = columns_[colIndex];
    const int z = previewZoom();
    col.preview->setPixmap(renderPreview(tiles_[col.cur], z));
    col.info->setText(tr("%1\nid %2  alpha %3")
                      .arg(QFileInfo(tiles_[col.cur].tsx).fileName())
                      .arg(tiles_[col.cur].id)
                      .arg(tiles_[col.cur].alpha_sum));
    renderTilesetView(col.tileset, col.scroll, tiles_[col.cur], highlightOn, -1);
}

void TileDeduplicator::showPrompt(int source)
{
    step_timer_->stop();
    buildCluster(source);
    showCluster();
}

// Build and paint the columns for the current pending set (cluster_) and show the
// decision view. Called for a fresh prompt, after each group merge (refreshed with
// the leftovers), and by BACK (restored from an undo snapshot).
void TileDeduplicator::showCluster()
{
    rebuildColumns();
    anim_on_ = true;
    int c = 0;
    while(c < static_cast<int>(columns_.size()))
    {
        renderColumn(c, anim_on_);
        c++;
    }
    anim_timer_->start();
    back_button_->setEnabled(!undo_.empty());
    decision_widget_->show();
}

// "KEEP THIS TILE": this column's tile is the group keeper (the master, never
// replaced). Every other TICKED column is merged into it and its maps repointed.
// The keeper STAYS in the list (so more tiles can be grouped into it) along with
// the untouched tiles; only the merged-away tiles drop out, and the view refreshes.
// When 1 or 0 tiles remain, scanning continues. One undo step covers the whole
// group (BACK restores the pending set and unmerges).
void TileDeduplicator::keepColumn(int masterIndex)
{
    if(masterIndex < 0 || masterIndex >= static_cast<int>(columns_.size()))
        return;
    const int keeper = columns_[masterIndex].cur;
    pushUndo();                // captures the pending set + state for BACK
    const std::size_t before = merge_log_.size();
    int c = 0;
    while(c < static_cast<int>(columns_.size()))
    {
        if(c != masterIndex && columns_[c].check != nullptr && columns_[c].check->isChecked())
        {
            const int loser = columns_[c].cur;
            // Skip a ticked column that resolves to the keeper or is already merged
            // away (e.g. two columns re-picked to the same tile).
            if(loser != keeper && tiles_[loser].merged_into == -1)
            {
                tiles_[loser].merged_into = keeper;
                MergeRec rec;
                rec.loser = loser;
                rec.keeper = keeper;
                rec.automatic = false;   // user decision (group merge)
                merge_log_.push_back(rec);
                similar_merges_++;
            }
        }
        c++;
    }
    if(merge_log_.size() == before)
    {
        undo_.pop_back();      // nothing ticked / nothing merged: keep the prompt as-is
        return;
    }
    effectiveSheets_.clear();  // merges changed: rebuild the "with changes" sheets

    // New pending set = every column whose tile is still active: the kept master
    // and the untouched tiles stay (so you can keep grouping into the master); only
    // the just-merged tiles drop out.
    std::vector<int> next;
    c = 0;
    while(c < static_cast<int>(columns_.size()))
    {
        if(tiles_[columns_[c].cur].merged_into == -1)
            next.push_back(columns_[c].cur);
        c++;
    }
    cluster_.swap(next);

    if(static_cast<int>(cluster_.size()) >= 2)
        showCluster();         // refresh: keeper + leftovers, ready for the next group
    else
        finishCluster();       // 1 or 0 left: nothing to group, resume scanning
}

// The pending set is exhausted (or SKIP): close the prompt and resume the O(n^2)
// scan from the next source. Any lone leftover tile is kept as-is; it is reconsidered
// when it later becomes a source.
void TileDeduplicator::finishCluster()
{
    flushChanges();            // commit this cluster's merges to disk (maps + png + log)
    undo_.clear();             // committed: BACK can no longer reach across this point
    decision_widget_->hide();
    anim_timer_->stop();
    clearColumns();
    cluster_.clear();
    advanceSource();           // the source is fully handled; move to the next one
    step_timer_->start();
}

void TileDeduplicator::onKeepColumnClicked()
{
    QObject *s = sender();
    if(s == nullptr)
        return;
    bool ok = false;
    const int col = s->property("col").toInt(&ok);
    if(ok)
        keepColumn(col);
}

// Magnification for the previews from a column's current on-screen size, so they
// grow when the window is maximized; never below the base kZoom.
int TileDeduplicator::previewZoom() const
{
    if(columns_.empty())
        return kZoom;
    int s = columns_[0].preview->width();
    if(columns_[0].preview->height() < s)
        s = columns_[0].preview->height();
    int z = s / kTileW;
    if(z < kZoom)
        z = kZoom;
    return z;
}

bool TileDeduplicator::eventFilter(QObject *watched, QEvent *event)
{
    if(event->type() == QEvent::MouseButtonPress && decision_widget_->isVisible())
    {
        int c = 0;
        while(c < static_cast<int>(columns_.size()))
        {
            if(watched == columns_[c].preview)
            {
                // click the big preview = toggle this column's group membership
                if(columns_[c].check != nullptr)
                    columns_[c].check->toggle();
                return true;
            }
            if(watched == columns_[c].tileset)
            {
                selectInColumn(c, static_cast<QMouseEvent *>(event)->position().toPoint());
                return true;
            }
            c++;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void TileDeduplicator::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // Re-render every column's preview crisp at the new size while a prompt is shown.
    if(decision_widget_->isVisible())
    {
        const int z = previewZoom();
        int c = 0;
        while(c < static_cast<int>(columns_.size()))
        {
            columns_[c].preview->setPixmap(renderPreview(tiles_[columns_[c].cur], z));
            c++;
        }
    }
}

void TileDeduplicator::onSkipClicked()
{
    // "Leave these alone": remember every tile currently shown so none of them is
    // ever offered again (as source or candidate), and persist each to the log so the
    // decision survives a restart. finishCluster() then commits any groups already
    // made in this cluster and moves on (it clears undo, so SKIP is not revertible).
    int c = 0;
    while(c < static_cast<int>(columns_.size()))
    {
        const int idx = columns_[c].cur;
        if(skipped_tiles_.insert(tileKey(idx)).second)
            logSkip(idx);
        c++;
    }
    skips_++;
    finishCluster();
}

void TileDeduplicator::pushUndo()
{
    UndoStep u;
    u.i = i_;
    u.j = j_;
    u.mergeLogLen = merge_log_.size();
    u.identical = identical_merges_;
    u.similar = similar_merges_;
    u.skips = skips_;
    u.cluster = cluster_;      // snapshot the pending set so BACK can restore it
    undo_.push_back(u);
}

// Revert the most recent SIMILAR decision: undo every merge it (and the identical
// auto-merges that followed it before the next prompt) made, drop a skip it added,
// restore the iteration position and counters, and re-show that prompt. Clicking
// BACK repeatedly walks further back through the decision history.
void TileDeduplicator::onBackClicked()
{
    if(undo_.empty())
        return;
    step_timer_->stop();
    const UndoStep u = undo_.back();
    undo_.pop_back();
    // Only ever reverts uncommitted merges (the current cluster); committed ones are
    // already on disk and out of undo_'s reach.
    while(merge_log_.size() > u.mergeLogLen)
    {
        const int idx = merge_log_.back().loser;
        merge_log_.pop_back();
        tiles_[idx].merged_into = -1;
    }
    identical_merges_ = u.identical;
    similar_merges_ = u.similar;
    skips_ = u.skips;
    effectiveSheets_.clear();  // merges reverted: rebuild the "with changes" sheets
    i_ = u.i;
    j_ = u.j;
    // Restore the exact pending set shown before that decision (its merges are now
    // reverted, so those tiles are active again) and re-display it.
    cluster_ = u.cluster;
    showCluster();
}

// Persist the current merges/skips to disk now, without ending the run, so the
// user need not finish every prompt to save progress.
void TileDeduplicator::onAnimTick()
{
    if(!decision_widget_->isVisible() || columns_.empty())
        return;
    anim_on_ = !anim_on_;
    int c = 0;
    while(c < static_cast<int>(columns_.size()))
    {
        renderTilesetView(columns_[c].tileset, columns_[c].scroll, tiles_[columns_[c].cur], anim_on_, -1);
        c++;
    }
}

// Build (and cache) the tileset sheet "with the changes done": start from the
// in-memory sheet, then blank every tile merged away this session, so the prompt
// previews the deduplicated result. The cache is cleared on every merge. Committed
// merges are blanked in the on-disk .png by flushChanges(); uncommitted ones (the
// current cluster) are blanked here in memory only until the cluster is committed.
QPixmap TileDeduplicator::effectiveSheet(const QString &tsx) const
{
    QHash<QString, QPixmap>::const_iterator c = effectiveSheets_.constFind(tsx);
    if(c != effectiveSheets_.constEnd())
        return c.value();

    QHash<QString, QPixmap>::const_iterator s = sheets_.constFind(tsx);
    if(s == sheets_.constEnd())
        return QPixmap();

    QPixmap canvas = s.value();          // COW copy of the on-disk sheet
    const int cols = sheetCols_.value(tsx, 1);
    if(cols > 0)
    {
        QPainter painter(&canvas);
        // A tile merged away this session is deleted: its maps now point at the
        // keeper (which lives in another sheet), so its own cell is cleared to
        // fully transparent "empty" space. CompositionMode_Source writes the
        // 0-alpha pixels rather than blending, so the slot reads as removed.
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        const int n = static_cast<int>(tiles_.size());
        int idx = 0;
        while(idx < n)
        {
            if(tiles_[idx].tsx == tsx && tiles_[idx].merged_into != -1)
            {
                const int tx = (tiles_[idx].id % cols) * kTileW;
                const int ty = (tiles_[idx].id / cols) * kTileH;
                painter.fillRect(tx, ty, kTileW, kTileH, Qt::transparent);
            }
            idx++;
        }
        painter.end();
    }

    effectiveSheets_.insert(tsx, canvas);
    return canvas;
}

// Show tile t's whole tileset at 1x with a marker rectangle at the tile cell;
// the marker is bright on the highlightOn phase (blink), and the view is scrolled
// to bring the tile into view.
void TileDeduplicator::renderTilesetView(QLabel *label, QScrollArea *scroll, const TileEntry &t, bool highlightOn, int pickIdx) const
{
    // Show the sheet "with the changes done" (the in-memory buffer), not the raw
    // on-disk tileset: tiles merged away this session appear as their keeper.
    QPixmap canvas = effectiveSheet(t.tsx);
    if(canvas.isNull())
    {
        label->clear();
        return;
    }
    const int cols = sheetCols_.value(t.tsx, 1);
    const int col = (cols > 0) ? (t.id % cols) : 0;
    const int row = (cols > 0) ? (t.id / cols) : 0;
    const int tx = col * kTileW;
    const int ty = row * kTileH;

    QPainter painter(&canvas);           // canvas is a COW copy; QPainter detaches it
    QPen pen(highlightOn ? QColor(255, 40, 40) : QColor(255, 220, 0));
    pen.setWidth(highlightOn ? 2 : 1);
    painter.setPen(pen);
    painter.drawRect(tx, ty, kTileW - 1, kTileH - 1);
    if(highlightOn)
        painter.drawRect(tx - 2, ty - 2, kTileW + 3, kTileH + 3);
    // green box on a manually-picked tile in this sheet
    if(pickIdx >= 0 && pickIdx < static_cast<int>(tiles_.size()) && tiles_[pickIdx].tsx == t.tsx)
    {
        const int pcols = sheetCols_.value(t.tsx, 1);
        const int pcol = (pcols > 0) ? (tiles_[pickIdx].id % pcols) : 0;
        const int prow = (pcols > 0) ? (tiles_[pickIdx].id / pcols) : 0;
        QPen pickPen(QColor(40, 220, 40));
        pickPen.setWidth(2);
        painter.setPen(pickPen);
        painter.drawRect(pcol * kTileW, prow * kTileH, kTileW - 1, kTileH - 1);
    }
    painter.end();

    label->setPixmap(canvas);
    label->resize(canvas.size());
    // Centre the marked tile in the scroll viewport.
    scroll->ensureVisible(tx + kTileW / 2, ty + kTileH / 2,
                          scroll->viewport()->width() / 2, scroll->viewport()->height() / 2);
}

// A click inside a column's tileset re-picks which tile that column stands for
// (so you can fix a mis-detected candidate or pair a tile the auto-compare
// missed). The pick must stay within this column's own tileset and be an active,
// non-empty tile; KEEP THIS TILE then uses the new selection.
void TileDeduplicator::selectInColumn(int colIndex, const QPoint &pos)
{
    if(colIndex < 0 || colIndex >= static_cast<int>(columns_.size()))
        return;
    Column &col = columns_[colIndex];
    const QString tsx = tiles_[col.cur].tsx;
    const int cols = sheetCols_.value(tsx, 1);
    if(cols <= 0 || pos.x() < 0 || pos.y() < 0)
        return;
    const int tileId = (pos.y() / kTileH) * cols + (pos.x() / kTileW);
    const int idx = tile_index_.value(qMakePair(tsx, tileId), -1);
    // Not selectable: an empty/animated/absent tile (no index), or one already
    // merged away this session (it now shows as empty space).
    if(idx < 0 || tiles_[idx].merged_into != -1)
        return;
    col.cur = idx;
    renderColumn(colIndex, anim_on_);
}

// Persist the current cluster's pending changes when the window is closed, so work
// is never lost just because the run was not finished (there is no manual SAVE).
void TileDeduplicator::closeEvent(QCloseEvent *event)
{
    flushChanges();
    QWidget::closeEvent(event);
}

void TileDeduplicator::finish()
{
    step_timer_->stop();
    anim_timer_->stop();
    decision_widget_->hide();
    status_label_->setText(tr("Rewriting maps..."));
    status_label_->repaint();
    QCoreApplication::processEvents();

    flushChanges();   // commit any not-yet-written merges (maps + png + log)
    applyRemaps();     // final full pass: clears empty-tile cells across all maps

    qDebug() << "Done."
             << "identical-merges:" << identical_merges_
             << "similar-merges:" << similar_merges_
             << "skips:" << skips_
             << "maps-rewritten:" << maps_rewritten_
             << "maps-skipped(unresolved tileset):" << maps_skipped_
             << "empty-cells-cleared:" << cells_cleared_;
    close();
    // In batch mode no window was shown, so closing won't end the event loop.
    QCoreApplication::quit();
}

// ---------------------------------------------------------------------------
// --migrate-from / --migrate-to
// ---------------------------------------------------------------------------

// Rewrite every map under datapack_path_ that references the migrate-from tileset so
// Repoint, in a .tmx's raw TEXT, the <tileset> entry whose source resolves to fromCanon
// so it points at toAbs (relative to the map's directory). Only that one source string
// changes; every other <tileset> (e.g. an intentional unresolved missing.tsx) and all the
// base64/zstd-encoded layer data stay byte-for-byte identical — which is exactly why this
// is used for maps libtiled would corrupt on re-serialization. The migrate-from and the
// new target share the same tile layout, so each cell's firstgid (and thus its gid) stays
// valid without touching the layer data. Returns true if a matching entry was rewritten.
bool TileDeduplicator::repointTilesetSourceInText(const QString &tmxPath,
                                                  const QString &fromCanon, const QString &toAbs)
{
    QFile rf(tmxPath);
    if(!rf.open(QFile::ReadOnly)) { qWarning() << "Unable to read map" << tmxPath; return false; }
    QString text = QString::fromUtf8(rf.readAll());
    rf.close();

    const QString mapDir = QFileInfo(tmxPath).absolutePath();
    const QString newRel = QDir(mapDir).relativeFilePath(toAbs).toHtmlEscaped();

    // Scan every <tileset ... source="REL" ...> opening tag; resolve REL against the map
    // dir and rewrite the ones whose target is fromCanon. An unresolved source (missing
    // file) has an empty canonicalFilePath() and so never matches — it is left intact.
    QRegularExpression re(QStringLiteral("<tileset\\b[^>]*\\bsource=\"([^\"]*)\"[^>]*>"));
    QRegularExpressionMatchIterator it = re.globalMatch(text);
    // Collect (offset,len) of each source value to replace, then apply from the end so the
    // earlier offsets stay valid.
    std::vector<QPair<int, int> > hits;
    while(it.hasNext())
    {
        QRegularExpressionMatch m = it.next();
        const QString abs = QFileInfo(QDir(mapDir), m.captured(1)).canonicalFilePath();
        if(!abs.isEmpty() && abs == fromCanon)
            hits.push_back(qMakePair(m.capturedStart(1), m.capturedLength(1)));
    }
    if(hits.empty())
        return false;
    int k = (int)hits.size() - 1;
    while(k >= 0)
    {
        text.replace(hits[k].first, hits[k].second, newRel);
        k--;
    }

    QFile wf(tmxPath);
    if(!wf.open(QFile::WriteOnly)) { qWarning() << "Unable to write map" << tmxPath; return false; }
    wf.write(text.toUtf8());
    wf.close();
    return true;
}

// it uses the migrate-to tileset instead (keeping each cell's local tile id), then
// delete the migrate-from .tsx and its image. The two tilesets are assumed to share
// the same tile layout (id N -> id N). Maps with an unresolved tileset are repointed in
// text (so an intentional missing.tsx is preserved) instead of being skipped.
void TileDeduplicator::runMigrate()
{
    const QString fromCanon = QFileInfo(migrate_from_).canonicalFilePath();
    const QString toCanon = QFileInfo(migrate_to_).canonicalFilePath();
    if(fromCanon.isEmpty()) { qWarning() << "migrate-from not found:" << migrate_from_; return; }
    if(toCanon.isEmpty())   { qWarning() << "migrate-to not found:"   << migrate_to_;   return; }
    if(fromCanon == toCanon) { qWarning() << "migrate-from and migrate-to are the same tileset"; return; }

    // Resolve the migrate-from image so it can be deleted along with the .tsx.
    QString fromPng;
    {
        Tiled::MapReader r;
        Tiled::SharedTileset ts = r.readTileset(fromCanon);
        if(!ts.isNull())
            fromPng = ts->imageSource().toLocalFile();
    }

    QStringList tmxFiles;
    {
        QDirIterator it(datapack_path_, QStringList() << QStringLiteral("*.tmx"),
                        QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext())
            tmxFiles.append(it.next());
    }
    tmxFiles.sort();

    int rewritten = 0;
    int skipped = 0;
    int fi = 0;
    while(fi < tmxFiles.size())
    {
        const QString &tmx = tmxFiles.at(fi);
        fi++;
        Tiled::MapReader reader;
        std::unique_ptr<Tiled::Map> map = reader.readMap(tmx);
        if(!map)
        {
            qWarning() << "Unable to read map" << tmx << ":" << reader.errorString();
            continue;
        }
        // Locate the from/to tilesets in this map. A map with a dangling tileset can't be
        // re-serialized by libtiled (firstgids would shift); it is repointed in text below.
        Tiled::SharedTileset oldTs;
        Tiled::SharedTileset newTs;
        bool dangling = false;
        int ti = 0;
        while(ti < map->tilesetCount())
        {
            Tiled::SharedTileset ts = map->tilesetAt(ti);
            const QString fn = ts->fileName();
            if(!fn.isEmpty() && !QFileInfo::exists(fn))
                dangling = true;
            else if(!fn.isEmpty() && !ts->isCollection() && ts->image().isNull())
                dangling = true;
            const QString c = QFileInfo(fn).canonicalFilePath();
            if(c == fromCanon)
                oldTs = ts;
            else if(c == toCanon)
                newTs = ts;
            ti++;
        }
        if(oldTs.isNull())
            continue;          // this map does not use the migrate-from tileset
        if(dangling)
        {
            // This map carries an unresolved tileset (e.g. an intentional missing.tsx), so
            // libtiled can't re-serialize it safely. Repoint the migrate-from <tileset>
            // entry in the TEXT instead — but only when the map does NOT already use the
            // migrate-to tileset (merging the two would need a gid remap we can't do here).
            if(!newTs.isNull())
            {
                qWarning() << "Skipping" << tmx << "- unresolved tileset AND already uses the migrate-to tileset (needs a gid merge)";
                skipped++;
                continue;
            }
            if(repointTilesetSourceInText(tmx, fromCanon, toCanon))
                rewritten++;
            else
            {
                qWarning() << "Skipping" << tmx << "- unresolved tileset and migrate-from entry not found in text";
                skipped++;
            }
            continue;
        }
        if(newTs.isNull())
        {
            // The map does not reference the migrate-to tileset yet: load it (with its
            // image so its tile count is right) and let replaceTileset() slot it in.
            Tiled::MapReader tr;
            newTs = tr.readTileset(toCanon);
            if(!newTs.isNull())
                newTs->loadImage();
            if(newTs.isNull())
            {
                qWarning() << "Unable to load migrate-to tileset" << toCanon;
                continue;
            }
            newTs->setFileName(toCanon);   // external reference (written relative)
        }
        map->replaceTileset(oldTs, newTs); // remaps cells (same id) and merges if both present

        Tiled::MapWriter writer;
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        writer.writeMap(map.get(), &buffer, QFileInfo(tmx).absolutePath());
        buffer.close();
        if(bytes.isEmpty())
        {
            qWarning() << "Failed to serialize map" << tmx;
            continue;
        }
        QFile f(tmx);
        if(f.open(QFile::WriteOnly))
        {
            f.write(bytes);
            f.close();
            rewritten++;
        }
        else
            qWarning() << "Unable to write map" << tmx;
    }

    // Delete the migrate-from tileset and its image (after no map references them).
    if(!fromPng.isEmpty() && QFile::exists(fromPng) && !QFile::remove(fromPng))
        qWarning() << "Unable to remove image" << fromPng;
    if(QFile::exists(fromCanon) && !QFile::remove(fromCanon))
        qWarning() << "Unable to remove tileset" << fromCanon;

    qDebug() << "Migrate done." << migrate_from_ << "->" << migrate_to_
             << "maps-rewritten:" << rewritten << "maps-skipped:" << skipped
             << "deleted:" << fromCanon << fromPng;
}

// Empty every cell that references the given tileset (recursing into groups; tile
// layers and object cells), counting how many were cleared.
void TileDeduplicator::clearTilesetCells(const QList<Tiled::Layer *> &layers, const Tiled::Tileset *ts, int &cleared)
{
    int li = 0;
    while(li < layers.size())
    {
        Tiled::Layer *layer = layers.at(li);
        if(Tiled::TileLayer *tl = layer->asTileLayer())
        {
            Tiled::TileLayer::iterator it = tl->begin();
            Tiled::TileLayer::iterator end = tl->end();
            while(it != end)
            {
                Tiled::Cell &cell = *it;
                if(!cell.isEmpty() && cell.tileset() == ts)
                {
                    cell = Tiled::Cell();
                    cleared++;
                }
                ++it;
            }
        }
        else if(Tiled::ObjectGroup *og = layer->asObjectGroup())
        {
            const QList<Tiled::MapObject *> &objs = og->objects();
            int oi = 0;
            while(oi < objs.size())
            {
                Tiled::MapObject *object = objs.at(oi);
                if(!object->cell().isEmpty() && object->cell().tileset() == ts)
                {
                    object->setCell(Tiled::Cell());
                    cleared++;
                }
                oi++;
            }
        }
        else if(Tiled::GroupLayer *gl = layer->asGroupLayer())
            clearTilesetCells(gl->layers(), ts, cleared);
        li++;
    }
}

// --remove: delete the given .tsx (and its image), clearing every cell that used it in
// every map and dropping its <tileset> entry. Maps with a dangling tileset are skipped.
void TileDeduplicator::runRemove()
{
    const QString rmCanon = QFileInfo(remove_tsx_).canonicalFilePath();
    if(rmCanon.isEmpty()) { qWarning() << "remove: tileset not found:" << remove_tsx_; return; }

    QString rmPng;
    {
        Tiled::MapReader r;
        Tiled::SharedTileset ts = r.readTileset(rmCanon);
        if(!ts.isNull())
            rmPng = ts->imageSource().toLocalFile();
    }

    QStringList tmxFiles;
    {
        QDirIterator it(datapack_path_, QStringList() << QStringLiteral("*.tmx"),
                        QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext())
            tmxFiles.append(it.next());
    }
    tmxFiles.sort();

    int rewritten = 0;
    int skipped = 0;
    int cellsCleared = 0;
    int fi = 0;
    while(fi < tmxFiles.size())
    {
        const QString &tmx = tmxFiles.at(fi);
        fi++;
        Tiled::MapReader reader;
        std::unique_ptr<Tiled::Map> map = reader.readMap(tmx);
        if(!map)
        {
            qWarning() << "Unable to read map" << tmx << ":" << reader.errorString();
            continue;
        }
        int rmIndex = -1;
        bool dangling = false;
        int ti = 0;
        while(ti < map->tilesetCount())
        {
            Tiled::SharedTileset ts = map->tilesetAt(ti);
            const QString fn = ts->fileName();
            if(!fn.isEmpty() && !QFileInfo::exists(fn))
                dangling = true;
            else if(!fn.isEmpty() && !ts->isCollection() && ts->image().isNull())
                dangling = true;
            if(QFileInfo(fn).canonicalFilePath() == rmCanon)
                rmIndex = ti;
            ti++;
        }
        if(rmIndex < 0)
            continue;          // this map does not reference the tileset to remove
        if(dangling)
        {
            qWarning() << "Skipping" << tmx << "- has a missing/unresolved tileset, left untouched";
            skipped++;
            continue;
        }
        const Tiled::Tileset *rmTs = map->tilesetAt(rmIndex).data();
        clearTilesetCells(map->layers(), rmTs, cellsCleared);
        map->removeTilesetAt(rmIndex);

        Tiled::MapWriter writer;
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        writer.writeMap(map.get(), &buffer, QFileInfo(tmx).absolutePath());
        buffer.close();
        if(bytes.isEmpty())
        {
            qWarning() << "Failed to serialize map" << tmx;
            continue;
        }
        QFile f(tmx);
        if(f.open(QFile::WriteOnly))
        {
            f.write(bytes);
            f.close();
            rewritten++;
        }
        else
            qWarning() << "Unable to write map" << tmx;
    }

    if(!rmPng.isEmpty() && QFile::exists(rmPng) && !QFile::remove(rmPng))
        qWarning() << "Unable to remove image" << rmPng;
    if(QFile::exists(rmCanon) && !QFile::remove(rmCanon))
        qWarning() << "Unable to remove tileset" << rmCanon;

    qDebug() << "Remove done." << remove_tsx_
             << "maps-rewritten:" << rewritten << "maps-skipped:" << skipped
             << "cells-cleared:" << cellsCleared << "deleted:" << rmCanon << rmPng;
}

// --move-from/--move-to: physically relocate a .tsx to a new path WITHOUT changing its
// tiles, then rewrite every .tmx under the scan path so its <tileset source> points at
// the new RELATIVE path. The tileset's image is NOT moved: the relocated .tsx keeps the
// SAME image, with its <image source> rewritten relative to the new .tsx location (the
// rest of the .tsx is copied byte-for-byte, so animations/properties survive untouched).
void TileDeduplicator::runMove()
{
    const QString fromCanon = QFileInfo(move_from_).canonicalFilePath();
    if(fromCanon.isEmpty()) { qWarning() << "move-from not found:" << move_from_; return; }

    // Destination: relative paths resolve against the current working directory (where
    // the user typed the command), absolute paths are used as-is. cleanPath collapses
    // a/b/../c without requiring the file to exist yet.
    const QString toAbs = QDir::cleanPath(QFileInfo(QDir::current(), move_to_).absoluteFilePath());
    if(toAbs == QDir::cleanPath(fromCanon))
    { qWarning() << "move-from and move-to are the same file"; return; }
    if(QFileInfo::exists(toAbs))
    { qWarning() << "move-to already exists, refusing to overwrite:" << toAbs; return; }
    if(!toAbs.endsWith(QStringLiteral(".tsx")))
    { qWarning() << "move-to must end in .tsx:" << toAbs; return; }

    // Read the source .tsx to learn its absolute image path (toLocalFile() resolves the
    // <image source> relative to the .tsx dir) and to grab the exact source string that
    // appears in the file, so the patch below replaces it precisely.
    QString absImage;
    {
        Tiled::MapReader r;
        Tiled::SharedTileset ts = r.readTileset(fromCanon);
        if(ts.isNull())
        { qWarning() << "Unable to read tileset" << fromCanon << ":" << r.errorString(); return; }
        absImage = ts->imageSource().toLocalFile();
    }

    // Read the source .tsx bytes verbatim and rewrite ONLY the first <image source="...">
    // value to the image's path relative to the NEW .tsx directory (the image stays put).
    QByteArray tsxBytes;
    {
        QFile f(fromCanon);
        if(!f.open(QFile::ReadOnly))
        { qWarning() << "Unable to read tileset file" << fromCanon; return; }
        tsxBytes = f.readAll();
        f.close();
    }
    const QString toDir = QFileInfo(toAbs).absolutePath();
    // Where the image ends up: with --move-image it moves into the destination dir (the new
    // .tsx then references it by bare filename); otherwise it stays put and the .tsx keeps a
    // relative path back to it.
    const QString newImageAbs = (move_image_ && !absImage.isEmpty())
                                    ? toDir + QLatin1Char('/') + QFileInfo(absImage).fileName()
                                    : absImage;
    QString tsxText = QString::fromUtf8(tsxBytes);
    if(!absImage.isEmpty())
    {
        const QString newRel = QDir(toDir).relativeFilePath(newImageAbs);
        // Match the first <image ... source="VALUE"> and replace just VALUE. Path strings
        // contain no XML metacharacters, but escape defensively for the attribute context.
        QRegularExpression re(QStringLiteral("(<image\\b[^>]*\\bsource=\")([^\"]*)(\")"));
        QRegularExpressionMatch m = re.match(tsxText);
        if(m.hasMatch())
            tsxText.replace(m.capturedStart(2), m.capturedLength(2), newRel.toHtmlEscaped());
        else
            qWarning() << "No <image source> found in" << fromCanon << "- image path left as-is";
    }

    // Create the destination directory and write the relocated .tsx.
    if(!QDir().mkpath(toDir))
    { qWarning() << "Unable to create destination directory:" << toDir; return; }
    {
        QFile f(toAbs);
        if(!f.open(QFile::WriteOnly))
        { qWarning() << "Unable to write moved tileset" << toAbs; return; }
        f.write(tsxText.toUtf8());
        f.close();
    }
    // Both old and new .tsx now exist; keep the old one until every referencing map has
    // been repointed, so the map reader can still resolve the original reference.

    QStringList tmxFiles;
    {
        QDirIterator it(datapack_path_, QStringList() << QStringLiteral("*.tmx"),
                        QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext())
            tmxFiles.append(it.next());
    }
    tmxFiles.sort();

    int rewritten = 0;
    int skipped = 0;
    int fi = 0;
    while(fi < tmxFiles.size())
    {
        const QString &tmx = tmxFiles.at(fi);
        fi++;
        Tiled::MapReader reader;
        std::unique_ptr<Tiled::Map> map = reader.readMap(tmx);
        if(!map)
        {
            qWarning() << "Unable to read map" << tmx << ":" << reader.errorString();
            continue;
        }
        // Find the tileset by its OLD canonical path. Maps with a dangling tileset can't
        // go through libtiled (it would shift firstgids), so they are repointed in text below.
        Tiled::SharedTileset target;
        bool dangling = false;
        int ti = 0;
        while(ti < map->tilesetCount())
        {
            Tiled::SharedTileset ts = map->tilesetAt(ti);
            const QString fn = ts->fileName();
            if(!fn.isEmpty() && !QFileInfo::exists(fn))
                dangling = true;
            else if(!fn.isEmpty() && !ts->isCollection() && ts->image().isNull())
                dangling = true;
            if(QFileInfo(fn).canonicalFilePath() == fromCanon)
                target = ts;
            ti++;
        }
        if(target.isNull())
            continue;          // this map does not reference the moved tileset
        if(dangling)
        {
            // Unresolved tileset present (e.g. an intentional missing.tsx): repoint the
            // moved tileset's <tileset source> in TEXT, preserving the dangling entry and
            // every encoded gid. A move never merges tilesets, so this is always safe.
            if(repointTilesetSourceInText(tmx, fromCanon, toAbs))
                rewritten++;
            else
            {
                qWarning() << "Skipping" << tmx << "- unresolved tileset and moved entry not found in text";
                skipped++;
            }
            continue;
        }
        // Point this map's tileset at the new path; MapWriter recomputes <tileset source>
        // relative to the map's own directory, so each map gets its correct relative path.
        target->setFileName(toAbs);

        Tiled::MapWriter writer;
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        writer.writeMap(map.get(), &buffer, QFileInfo(tmx).absolutePath());
        buffer.close();
        if(bytes.isEmpty())
        {
            qWarning() << "Failed to serialize map" << tmx;
            continue;
        }
        QFile f(tmx);
        if(f.open(QFile::WriteOnly))
        {
            f.write(bytes);
            f.close();
            rewritten++;
        }
        else
            qWarning() << "Unable to write map" << tmx;
    }

    // --move-image: relocate the image file alongside the .tsx (the new .tsx already
    // references it by bare filename). Done after the maps are rewritten; the maps point at
    // the .tsx, not the image, so ordering is irrelevant to them.
    if(move_image_ && !absImage.isEmpty() && newImageAbs != absImage)
    {
        if(QFile::exists(newImageAbs))
            qWarning() << "Image destination already exists, left image in place:" << newImageAbs;
        else if(!QFile::rename(absImage, newImageAbs))
            qWarning() << "Unable to move image" << absImage << "->" << newImageAbs;
    }

    // Delete the original .tsx only when every referencing map was repointed. If any map
    // was skipped (dangling), keep the old .tsx so that map's reference stays resolvable.
    if(skipped == 0)
    {
        if(QFile::exists(fromCanon) && !QFile::remove(fromCanon))
            qWarning() << "Unable to remove original tileset" << fromCanon;
    }
    else
        qWarning() << "Kept original" << fromCanon << "-" << skipped
                   << "map(s) with a dangling tileset still reference it";

    qDebug() << "Move done." << fromCanon << "->" << toAbs
             << "maps-rewritten:" << rewritten << "maps-skipped:" << skipped
             << "image:" << newImageAbs;
}

namespace {
// TMX global tile ids carry the three flip bits in the top nibble; the rest is the id.
const quint32 kGidFlags = 0xE0000000u;
const quint32 kGidId    = 0x1FFFFFFFu;

quint32 rdLE32(const char *p)
{
    const unsigned char *u = reinterpret_cast<const unsigned char *>(p);
    return (quint32)u[0] | ((quint32)u[1] << 8) | ((quint32)u[2] << 16) | ((quint32)u[3] << 24);
}
void wrLE32(char *p, quint32 v)
{
    unsigned char *u = reinterpret_cast<unsigned char *>(p);
    u[0] = (unsigned char)(v & 0xFF);
    u[1] = (unsigned char)((v >> 8) & 0xFF);
    u[2] = (unsigned char)((v >> 16) & 0xFF);
    u[3] = (unsigned char)((v >> 24) & 0xFF);
}

// Decode one base64+zstd tile-layer <data> blob into `count` little-endian gids.
std::vector<quint32> decodeLayer(const QString &base64, int count)
{
    QByteArray comp = QByteArray::fromBase64(base64.trimmed().toLatin1());
    QByteArray raw = Tiled::decompress(comp, count * 4, Tiled::Zstandard);
    std::vector<quint32> gids;
    if(raw.size() != count * 4)
        return gids;
    gids.resize((std::size_t)count);
    int i = 0;
    while(i < count) { gids[(std::size_t)i] = rdLE32(raw.constData() + i * 4); i++; }
    return gids;
}
// Re-encode a gid array back to base64(zstd), the same format the maps already use.
QString encodeLayer(const std::vector<quint32> &gids)
{
    QByteArray raw;
    raw.resize((int)gids.size() * 4);
    int i = 0;
    while(i < (int)gids.size()) { wrLE32(raw.data() + i * 4, gids[(std::size_t)i]); i++; }
    QByteArray comp = Tiled::compress(raw, Tiled::Zstandard, -1);
    return QString::fromLatin1(comp.toBase64());
}

int attrInt(const QString &tag, const QString &name)
{
    QRegularExpression r(QStringLiteral("\\b") + name + QStringLiteral("=\"(\\d+)\""));
    QRegularExpressionMatch m = r.match(tag);
    return m.hasMatch() ? m.captured(1).toInt() : 0;
}

// One external <tileset firstgid=.. source=..> reference in a map: where its line lives in
// the text, and whether to merge it away (resolvable & not kept) or leave it (kept/unresolved).
struct MapTileset
{
    quint64 firstgid;
    QString canon;      // canonical .tsx path, or "" when unresolved (a dangling source)
    bool merge;
    int lineStart;      // start of the whole <tileset .../> line (incl. leading whitespace)
    int lineEnd;        // just past the trailing newline
};
bool mapTilesetLess(const MapTileset &a, const MapTileset &b) { return a.firstgid < b.firstgid; }

// Parse a map's external tileset table from its text. keepBasenames are left as separate
// references (never merged); unresolved sources (canon == "") are likewise left untouched.
std::vector<MapTileset> parseMapTilesets(const QString &text, const QString &mapDir,
                                         const QSet<QString> &keepBasenames)
{
    std::vector<MapTileset> out;
    QRegularExpression tagRe(QStringLiteral("<tileset\\b[^>]*>"));
    QRegularExpression fgRe(QStringLiteral("\\bfirstgid=\"(\\d+)\""));
    QRegularExpression srcRe(QStringLiteral("\\bsource=\"([^\"]*)\""));
    QRegularExpressionMatchIterator it = tagRe.globalMatch(text);
    while(it.hasNext())
    {
        QRegularExpressionMatch m = it.next();
        const QString tag = m.captured(0);
        QRegularExpressionMatch fg = fgRe.match(tag);
        QRegularExpressionMatch sm = srcRe.match(tag);
        if(!fg.hasMatch() || !sm.hasMatch())
            continue;                       // embedded (non-external) tileset: skip
        MapTileset e;
        e.firstgid = fg.captured(1).toULongLong();
        const QString src = sm.captured(1);
        e.canon = QFileInfo(QDir(mapDir), src).canonicalFilePath();
        e.merge = !e.canon.isEmpty() && !keepBasenames.contains(QFileInfo(src).fileName());
        int ls = m.capturedStart(0);
        while(ls > 0 && text.at(ls - 1) != QLatin1Char('\n')) ls--;
        int le = m.capturedEnd(0);
        while(le < text.size() && text.at(le) != QLatin1Char('\n')) le++;
        if(le < text.size()) le++;          // consume the newline so a removed line leaves none
        e.lineStart = ls;
        e.lineEnd = le;
        out.push_back(e);
    }
    return out;
}

// Index (into `sorted`, ascending by firstgid) of the tileset owning realgid; -1 if none.
int owningTileset(const std::vector<MapTileset> &sorted, quint32 realgid)
{
    int i = 0, found = -1;
    while(i < (int)sorted.size() && (quint64)realgid >= sorted[(std::size_t)i].firstgid)
    {
        found = i;
        i++;
    }
    return found;
}

quint32 remapGid(quint32 gid, const std::vector<MapTileset> &sorted, quint64 mergedFirstgid,
                 const QHash<QString, int> &slotByKey)
{
    if(gid == 0)
        return 0;
    const quint32 flags = gid & kGidFlags;
    const quint32 real = gid & kGidId;
    const int idx = owningTileset(sorted, real);
    if(idx < 0 || !sorted[(std::size_t)idx].merge)
        return gid;                         // kept or unresolved tileset: gid unchanged
    const int localid = (int)(real - sorted[(std::size_t)idx].firstgid);
    const int slot = slotByKey.value(sorted[(std::size_t)idx].canon + QLatin1Char('#') + QString::number(localid), -1);
    if(slot < 0)
        return gid;                         // not collected (shouldn't happen): leave as-is
    return ((quint32)(mergedFirstgid + (quint64)slot)) | flags;
}

// All state of the in-progress merge: the per-tileset image cache and the
// identical-tile-collapsing slot assignment.
struct MergeState
{
    QHash<QString, QImage> pngByCanon;      // canonical .tsx -> its image (ARGB32)
    QHash<QString, int> colsByCanon;        // canonical .tsx -> columns (image width / 16)
    QHash<QByteArray, int> slotByPixels;    // 16x16 pixels -> slot (identical tiles collapse)
    QHash<QString, int> slotByKey;          // "canon#localid" -> slot
    std::vector<QImage> uniques;            // the unique 16x16 tiles, in slot order
};

// Extract a 16x16 tile from its tileset image (loading+caching the image on first use), then
// assign it a slot — reusing an existing slot when the pixels are identical to one already kept.
void recordUse(MergeState &st, const QString &canon, int localid)
{
    const QString key = canon + QLatin1Char('#') + QString::number(localid);
    if(st.slotByKey.contains(key))
        return;
    if(!st.pngByCanon.contains(canon))
    {
        Tiled::MapReader r;
        Tiled::SharedTileset ts = r.readTileset(canon);
        QImage img;
        if(!ts.isNull())
        {
            ts->loadImage();
            img = QImage(ts->imageSource().toLocalFile());
            if(!img.isNull())
                img = img.convertToFormat(QImage::Format_ARGB32);
        }
        st.pngByCanon.insert(canon, img);
        st.colsByCanon.insert(canon, img.isNull() ? 0 : img.width() / 16);
    }
    const QImage &img = st.pngByCanon[canon];
    const int cols = st.colsByCanon[canon];
    if(img.isNull() || cols <= 0)
        return;
    const int x = (localid % cols) * 16;
    const int y = (localid / cols) * 16;
    if(x + 16 > img.width() || y + 16 > img.height())
    {
        qWarning() << "merge: tile" << localid << "out of range in" << canon;
        return;
    }
    QImage tile = img.copy(x, y, 16, 16);
    const QByteArray pixels(reinterpret_cast<const char *>(tile.constBits()), (int)tile.sizeInBytes());
    int slot = st.slotByPixels.value(pixels, -1);
    if(slot < 0)
    {
        slot = (int)st.uniques.size();
        st.uniques.push_back(tile);
        st.slotByPixels.insert(pixels, slot);
    }
    st.slotByKey.insert(key, slot);
}

// Collect every used tile (tile-layer cells + object gids) of one map into the merge state.
void collectMapUses(MergeState &st, const QString &text, const std::vector<MapTileset> &sorted)
{
    QRegularExpression layerRe(QStringLiteral("<layer\\b([^>]*)>([\\s\\S]*?)</layer>"));
    QRegularExpression dataRe(QStringLiteral("<data\\b[^>]*>([\\s\\S]*?)</data>"));
    QRegularExpressionMatchIterator lit = layerRe.globalMatch(text);
    while(lit.hasNext())
    {
        QRegularExpressionMatch lm = lit.next();
        const int w = attrInt(lm.captured(1), QStringLiteral("width"));
        const int h = attrInt(lm.captured(1), QStringLiteral("height"));
        QRegularExpressionMatch dm = dataRe.match(lm.captured(2));
        if(w <= 0 || h <= 0 || !dm.hasMatch())
            continue;
        std::vector<quint32> gids = decodeLayer(dm.captured(1), w * h);
        std::size_t i = 0;
        while(i < gids.size())
        {
            const quint32 real = gids[i] & kGidId;
            if(real != 0)
            {
                const int idx = owningTileset(sorted, real);
                if(idx >= 0 && sorted[(std::size_t)idx].merge)
                    recordUse(st, sorted[(std::size_t)idx].canon, (int)(real - sorted[(std::size_t)idx].firstgid));
            }
            i++;
        }
    }
    QRegularExpression objRe(QStringLiteral("<object\\b[^>]*?\\bgid=\"(\\d+)\""));
    QRegularExpressionMatchIterator oit = objRe.globalMatch(text);
    while(oit.hasNext())
    {
        const quint32 real = oit.next().captured(1).toUInt() & kGidId;
        if(real != 0)
        {
            const int idx = owningTileset(sorted, real);
            if(idx >= 0 && sorted[(std::size_t)idx].merge)
                recordUse(st, sorted[(std::size_t)idx].canon, (int)(real - sorted[(std::size_t)idx].firstgid));
        }
    }
}

// One pending text edit; applied from the end of the file so earlier offsets stay valid.
struct TextEdit { int start; int len; QString val; };
bool textEditGreater(const TextEdit &a, const TextEdit &b) { return a.start > b.start; }

} // namespace

// --merge-used: collapse the tiles actually used across every map under the path into ONE
// new tileset and rewrite the maps to it. See the header for the full contract.
void TileDeduplicator::runMergeUsed()
{
    if(merge_out_.isEmpty()) { qWarning() << "--merge-used needs --merge-out <path/.tsx>"; return; }
    const QString outAbs = QDir::cleanPath(QFileInfo(QDir::current(), merge_out_).absoluteFilePath());
    if(!outAbs.endsWith(QStringLiteral(".tsx"))) { qWarning() << "--merge-out must end in .tsx:" << outAbs; return; }
    const QString outDir = QFileInfo(outAbs).absolutePath();
    const QString outName = QFileInfo(outAbs).fileName();
    const QString outBase = QFileInfo(outAbs).completeBaseName();
    const QString outPng = outDir + QLatin1Char('/') + outBase + QStringLiteral(".png");

    QSet<QString> keep;
    int ki = 0;
    while(ki < merge_keep_.size()) { keep.insert(QFileInfo(merge_keep_.at(ki)).fileName()); ki++; }

    QStringList tmxFiles;
    {
        QDirIterator it(datapack_path_, QStringList() << QStringLiteral("*.tmx"),
                        QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext())
            tmxFiles.append(it.next());
    }
    tmxFiles.sort();

    // PASS 1 — collect every used tile from the merge-eligible tilesets, collapsing identical
    // tiles to one slot.
    MergeState st;
    int fi = 0;
    while(fi < tmxFiles.size())
    {
        const QString tmx = tmxFiles.at(fi);
        fi++;
        QFile f(tmx);
        if(!f.open(QFile::ReadOnly)) { qWarning() << "Unable to read map" << tmx; continue; }
        const QString text = QString::fromUtf8(f.readAll());
        f.close();
        std::vector<MapTileset> sorted = parseMapTilesets(text, QFileInfo(tmx).absolutePath(), keep);
        std::sort(sorted.begin(), sorted.end(), mapTilesetLess);
        collectMapUses(st, text, sorted);
    }
    const int uniqueCount = (int)st.uniques.size();
    if(uniqueCount == 0) { qWarning() << "merge: no used tiles found under" << datapack_path_; return; }

    // Build the single atlas image + its .tsx (identical tiles already collapsed).
    const int cols = 16;
    const int rows = (uniqueCount + cols - 1) / cols;
    QImage atlas(cols * 16, rows * 16, QImage::Format_ARGB32);
    atlas.fill(QColor(0, 0, 0));            // black == transparent via trans="000000" below
    {
        QPainter p(&atlas);
        int s = 0;
        while(s < uniqueCount)
        {
            p.drawImage((s % cols) * 16, (s / cols) * 16, st.uniques[(std::size_t)s]);
            s++;
        }
    }
    if(!QDir().mkpath(outDir)) { qWarning() << "merge: cannot create" << outDir; return; }
    if(!atlas.save(outPng, "PNG")) { qWarning() << "merge: cannot write" << outPng; return; }
    {
        QString tsx = QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        tsx += QStringLiteral(" <tileset name=\"%1\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"%2\" columns=\"%3\">\n")
                   .arg(outName).arg(uniqueCount).arg(cols);
        tsx += QStringLiteral("  <image source=\"%1.png\" trans=\"000000\" width=\"%2\" height=\"%3\"/>\n")
                   .arg(outBase).arg(cols * 16).arg(rows * 16);
        tsx += QStringLiteral(" </tileset>\n");
        QFile tf(outAbs);
        if(!tf.open(QFile::WriteOnly)) { qWarning() << "merge: cannot write" << outAbs; return; }
        tf.write(tsx.toUtf8());
        tf.close();
    }

    // PASS 2 — rewrite every map: remap merged-tileset gids to the single tileset, drop the
    // merged-away <tileset> entries, append the new one. Kept/unresolved tilesets, object
    // markers and untouched layers stay byte-for-byte identical.
    int mapsRewritten = 0;
    int mapsUntouched = 0;
    fi = 0;
    while(fi < tmxFiles.size())
    {
        const QString tmx = tmxFiles.at(fi);
        fi++;
        const QString mapDir = QFileInfo(tmx).absolutePath();
        QFile rf(tmx);
        if(!rf.open(QFile::ReadOnly)) { qWarning() << "Unable to read map" << tmx; continue; }
        QString text = QString::fromUtf8(rf.readAll());
        rf.close();

        std::vector<MapTileset> table = parseMapTilesets(text, mapDir, keep);
        std::vector<MapTileset> sorted = table;
        std::sort(sorted.begin(), sorted.end(), mapTilesetLess);
        bool anyMerge = false;
        std::size_t ti = 0;
        while(ti < table.size()) { if(table[ti].merge) anyMerge = true; ti++; }
        if(!anyMerge) { mapsUntouched++; continue; }   // this map uses none of the merged tilesets

        // The merged tileset's firstgid must sit above every kept gid and firstgid so it owns
        // its own range and never collides; find the map's maxima first.
        quint64 maxV = 0;
        ti = 0;
        while(ti < table.size()) { if(table[ti].firstgid > maxV) maxV = table[ti].firstgid; ti++; }
        QRegularExpression layerRe(QStringLiteral("<layer\\b([^>]*)>([\\s\\S]*?)</layer>"));
        QRegularExpression dataRe(QStringLiteral("<data\\b[^>]*>([\\s\\S]*?)</data>"));
        {
            QRegularExpressionMatchIterator lit = layerRe.globalMatch(text);
            while(lit.hasNext())
            {
                QRegularExpressionMatch lm = lit.next();
                const int w = attrInt(lm.captured(1), QStringLiteral("width"));
                const int h = attrInt(lm.captured(1), QStringLiteral("height"));
                QRegularExpressionMatch dm = dataRe.match(lm.captured(2));
                if(w <= 0 || h <= 0 || !dm.hasMatch())
                    continue;
                std::vector<quint32> gids = decodeLayer(dm.captured(1), w * h);
                std::size_t i = 0;
                while(i < gids.size()) { const quint32 r = gids[i] & kGidId; if((quint64)r > maxV) maxV = r; i++; }
            }
            QRegularExpression objRe(QStringLiteral("<object\\b[^>]*?\\bgid=\"(\\d+)\""));
            QRegularExpressionMatchIterator oit = objRe.globalMatch(text);
            while(oit.hasNext())
            {
                const quint32 r = oit.next().captured(1).toUInt() & kGidId;
                if((quint64)r > maxV) maxV = r;
            }
        }
        const quint64 mergedFirstgid = maxV + 1;

        std::vector<TextEdit> edits;

        // (a) rebuild the tileset table: keep non-merge entries verbatim, append the new one.
        int tableStart = text.size(), tableEnd = 0;
        ti = 0;
        while(ti < table.size())
        {
            if(table[ti].lineStart < tableStart) tableStart = table[ti].lineStart;
            if(table[ti].lineEnd > tableEnd) tableEnd = table[ti].lineEnd;
            ti++;
        }
        QString newTable;
        ti = 0;
        while(ti < table.size())
        {
            if(!table[ti].merge)
                newTable += text.mid(table[ti].lineStart, table[ti].lineEnd - table[ti].lineStart);
            ti++;
        }
        const QString rel = QDir(mapDir).relativeFilePath(outAbs).toHtmlEscaped();
        newTable += QStringLiteral(" <tileset firstgid=\"%1\" source=\"%2\"/>\n").arg(mergedFirstgid).arg(rel);
        TextEdit te; te.start = tableStart; te.len = tableEnd - tableStart; te.val = newTable;
        edits.push_back(te);

        // (b) remap each tile layer's gids (only re-encode layers that actually changed).
        {
            QRegularExpressionMatchIterator lit = layerRe.globalMatch(text);
            while(lit.hasNext())
            {
                QRegularExpressionMatch lm = lit.next();
                const int w = attrInt(lm.captured(1), QStringLiteral("width"));
                const int h = attrInt(lm.captured(1), QStringLiteral("height"));
                QRegularExpressionMatch dm = dataRe.match(lm.captured(2));
                if(w <= 0 || h <= 0 || !dm.hasMatch())
                    continue;
                std::vector<quint32> gids = decodeLayer(dm.captured(1), w * h);
                if((int)gids.size() != w * h)
                    continue;
                bool changed = false;
                std::size_t i = 0;
                while(i < gids.size())
                {
                    const quint32 ng = remapGid(gids[i], sorted, mergedFirstgid, st.slotByKey);
                    if(ng != gids[i]) { gids[i] = ng; changed = true; }
                    i++;
                }
                if(changed)
                {
                    TextEdit e2;
                    e2.start = lm.capturedStart(2) + dm.capturedStart(1);
                    e2.len = dm.capturedLength(1);
                    e2.val = QStringLiteral("\n  ") + encodeLayer(gids) + QStringLiteral("\n  ");
                    edits.push_back(e2);
                }
            }
        }

        // (c) remap object gids that point into a merged tileset (markers stay as they are).
        {
            QRegularExpression objRe(QStringLiteral("<object\\b[^>]*?\\bgid=\"(\\d+)\""));
            QRegularExpressionMatchIterator oit = objRe.globalMatch(text);
            while(oit.hasNext())
            {
                QRegularExpressionMatch om = oit.next();
                const quint32 g = om.captured(1).toUInt();
                const quint32 ng = remapGid(g, sorted, mergedFirstgid, st.slotByKey);
                if(ng != g)
                {
                    TextEdit e3; e3.start = om.capturedStart(1); e3.len = om.capturedLength(1);
                    e3.val = QString::number(ng);
                    edits.push_back(e3);
                }
            }
        }

        std::sort(edits.begin(), edits.end(), textEditGreater);
        std::size_t ei = 0;
        while(ei < edits.size()) { text.replace(edits[ei].start, edits[ei].len, edits[ei].val); ei++; }

        QFile wf(tmx);
        if(!wf.open(QFile::WriteOnly)) { qWarning() << "Unable to write map" << tmx; continue; }
        wf.write(text.toUtf8());
        wf.close();
        mapsRewritten++;
    }

    qDebug() << "Merge-used done." << "tileset:" << outAbs
             << "unique-tiles:" << uniqueCount << "(" << cols << "x" << rows << ")"
             << "maps-rewritten:" << mapsRewritten << "maps-untouched:" << mapsUntouched;
}

namespace {
// One --stat line: a tileset, how many tile cells reference it (repeats counted) and
// in how many maps. Sorted least-used first so cleanup candidates show on top.
struct StatRow
{
    qint64 tiles;
    int maps;
    QString path;
};
bool statRowLess(const StatRow &a, const StatRow &b)
{
    if(a.tiles != b.tiles)
        return a.tiles < b.tiles;
    if(a.maps != b.maps)
        return a.maps < b.maps;
    return a.path < b.path;
}
}

// Tally, for one map's layers, how many cells reference each tileset (tilesByTsx, by
// canonical .tsx, repeats counted) and which tilesets appear at all (seen, for the
// per-map count). Recurses into group layers; covers tile layers and object cells.
void TileDeduplicator::statCountLayers(const QList<Tiled::Layer *> &layers,
    const QHash<const Tiled::Tileset *, QString> &canonByTs,
    QHash<QString, qint64> &tilesByTsx, QSet<QString> &seen)
{
    int li = 0;
    while(li < layers.size())
    {
        Tiled::Layer *layer = layers.at(li);
        if(Tiled::TileLayer *tl = layer->asTileLayer())
        {
            Tiled::TileLayer::iterator it = tl->begin();
            Tiled::TileLayer::iterator end = tl->end();
            while(it != end)
            {
                const Tiled::Cell &cell = *it;
                if(!cell.isEmpty() && cell.tileset() != nullptr)
                {
                    const QString canon = canonByTs.value(cell.tileset());
                    if(!canon.isEmpty())
                    {
                        tilesByTsx[canon] += 1;
                        seen.insert(canon);
                    }
                }
                ++it;
            }
        }
        else if(Tiled::ObjectGroup *og = layer->asObjectGroup())
        {
            const QList<Tiled::MapObject *> &objs = og->objects();
            int oi = 0;
            while(oi < objs.size())
            {
                const Tiled::Cell &cell = objs.at(oi)->cell();
                if(!cell.isEmpty() && cell.tileset() != nullptr)
                {
                    const QString canon = canonByTs.value(cell.tileset());
                    if(!canon.isEmpty())
                    {
                        tilesByTsx[canon] += 1;
                        seen.insert(canon);
                    }
                }
                oi++;
            }
        }
        else if(Tiled::GroupLayer *gl = layer->asGroupLayer())
            statCountLayers(gl->layers(), canonByTs, tilesByTsx, seen);
        li++;
    }
}

// --stat: for every .tsx under datapack_path_, print how many tile cells (across all
// maps under the path, repeats counted) use it and in how many maps. Least-used first.
void TileDeduplicator::runStat()
{
    QStringList tsxFiles;
    {
        QDirIterator it(datapack_path_, QStringList() << QStringLiteral("*.tsx"),
                        QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext())
            tsxFiles.append(it.next());
    }
    QStringList tmxFiles;
    {
        QDirIterator it(datapack_path_, QStringList() << QStringLiteral("*.tmx"),
                        QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext())
            tmxFiles.append(it.next());
    }

    QHash<QString, qint64> tilesByTsx;   // canonical .tsx -> tile cells (repeats counted)
    QHash<QString, int> mapsByTsx;       // canonical .tsx -> maps that use it
    int fi = 0;
    while(fi < tmxFiles.size())
    {
        const QString &tmx = tmxFiles.at(fi);
        fi++;
        Tiled::MapReader reader;
        std::unique_ptr<Tiled::Map> map = reader.readMap(tmx);
        if(!map)
        {
            qWarning() << "Unable to read map" << tmx << ":" << reader.errorString();
            continue;
        }
        QHash<const Tiled::Tileset *, QString> canonByTs;
        int ti = 0;
        while(ti < map->tilesetCount())
        {
            Tiled::SharedTileset ts = map->tilesetAt(ti);
            QString c = QFileInfo(ts->fileName()).canonicalFilePath();
            if(c.isEmpty())
                c = QFileInfo(ts->fileName()).absoluteFilePath();
            canonByTs.insert(ts.data(), c);
            ti++;
        }
        QSet<QString> seen;
        statCountLayers(map->layers(), canonByTs, tilesByTsx, seen);
        QSet<QString>::const_iterator sit = seen.constBegin();
        while(sit != seen.constEnd())
        {
            mapsByTsx[*sit] += 1;
            ++sit;
        }
    }

    std::vector<StatRow> rows;
    int i = 0;
    while(i < tsxFiles.size())
    {
        const QString &tsx = tsxFiles.at(i);
        i++;
        QString canon = QFileInfo(tsx).canonicalFilePath();
        if(canon.isEmpty())
            canon = QFileInfo(tsx).absoluteFilePath();
        StatRow row;
        row.tiles = tilesByTsx.value(canon, 0);
        row.maps = mapsByTsx.value(canon, 0);
        row.path = QDir(datapack_path_).relativeFilePath(tsx);
        rows.push_back(row);
    }
    std::sort(rows.begin(), rows.end(), statRowLess);

    int r = 0;
    while(r < static_cast<int>(rows.size()))
    {
        std::cout << rows[r].path.toStdString() << "  "
                  << rows[r].tiles << " tiles used into "
                  << rows[r].maps << " maps" << std::endl;
        r++;
    }
    std::cout << "-- " << rows.size() << " tileset(s), " << tmxFiles.size() << " map(s)." << std::endl;
}

// ---------------------------------------------------------------------------
// .tmx rewriting
// ---------------------------------------------------------------------------

void TileDeduplicator::applyRemaps()
{
    // Build the final loser -> kept-root remap from the union-find state.
    remap_.clear();
    int idx = 0;
    while(idx < static_cast<int>(tiles_.size()))
    {
        if(tiles_[idx].merged_into != -1)
        {
            const int root = findRoot(idx);
            const QString key = tiles_[idx].tsx + QStringLiteral("#") + QString::number(tiles_[idx].id);
            remap_.insert(key, qMakePair(tiles_[root].tsx, tiles_[root].id));
        }
        idx++;
    }

    if(remap_.isEmpty() && transparent_tiles_.empty())
        return; // nothing merged and no empty tiles to clear: leave maps untouched

    QStringList tmxFiles;
    {
        QDirIterator it(datapack_path_, QStringList() << QStringLiteral("*.tmx"),
                        QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext())
            tmxFiles.append(it.next());
    }
    tmxFiles.sort();

    startWriter();
    int fi = 0;
    while(fi < tmxFiles.size())
    {
        remapOneMap(tmxFiles.at(fi));
        fi++;
    }
    stopWriterAndJoin();
}

// Commit every merge made since the last commit straight to disk: rewrite the
// affected maps to the keeper, blank the merged loser tiles to transparent in their
// tileset .png, then append the merges to the log. Order matters: maps are written
// BEFORE the png is blanked, so a crash in between leaves the loser tile intact (it
// is simply re-detected next run) rather than blanked-but-still-referenced.
void TileDeduplicator::flushChanges()
{
    if(merge_log_committed_ >= merge_log_.size())
        return;               // nothing new to persist
    if(!map_index_built_)
        buildMapIndex();

    // Rebuild the full loser -> kept-root remap from the union-find state.
    remap_.clear();
    int idx = 0;
    while(idx < static_cast<int>(tiles_.size()))
    {
        if(tiles_[idx].merged_into != -1)
        {
            const int root = findRoot(idx);
            const QString key = tiles_[idx].tsx + QStringLiteral("#") + QString::number(tiles_[idx].id);
            remap_.insert(key, qMakePair(tiles_[root].tsx, tiles_[root].id));
        }
        idx++;
    }

    // Blank each newly-merged loser tile to transparent in its tileset image (in
    // memory first), then note which sheets are now FULLY transparent: those tilesets
    // are dead (every tile merged away) and get deleted + unlinked below.
    QHash<QString, QImage> edited;
    std::size_t mm = merge_log_committed_;
    while(mm < merge_log_.size())
    {
        const int loser = merge_log_[mm].loser;
        const QString &tsx = tiles_[loser].tsx;
        if(sheets_.contains(tsx) && !png_by_tsx_.value(tsx).isEmpty())
        {
            if(!edited.contains(tsx))
                edited.insert(tsx, sheets_.value(tsx).toImage().convertToFormat(QImage::Format_ARGB32));
            QImage &img = edited[tsx];
            const int cols = sheetCols_.value(tsx, 1);
            if(cols > 0)
            {
                const int tx = (tiles_[loser].id % cols) * kTileW;
                const int ty = (tiles_[loser].id / cols) * kTileH;
                int y = ty;
                while(y < ty + kTileH && y < img.height())
                {
                    QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
                    int x = tx;
                    while(x < tx + kTileW && x < img.width())
                    {
                        line[x] = qRgba(0, 0, 0, 0);
                        x++;
                    }
                    y++;
                }
            }
        }
        mm++;
    }
    {
        QHash<QString, QImage>::const_iterator dit = edited.constBegin();
        while(dit != edited.constEnd())
        {
            if(isFullyTransparent(dit.value()))
                dead_tsx_.insert(dit.key().toStdString());
            ++dit;
        }
    }

    // Maps to rewrite: those that reference a newly-merged loser's tileset, plus those
    // that reference a newly-dead tileset (so its now-unused <tileset> entry is dropped).
    QStringList affected;
    {
        std::unordered_set<std::string> seen;
        std::size_t m = merge_log_committed_;
        while(m < merge_log_.size())
        {
            const QStringList &maps = maps_by_tsx_.value(tiles_[merge_log_[m].loser].tsx);
            int k = 0;
            while(k < maps.size())
            {
                if(seen.insert(maps.at(k).toStdString()).second)
                    affected.append(maps.at(k));
                k++;
            }
            m++;
        }
        QHash<QString, QImage>::const_iterator dit = edited.constBegin();
        while(dit != edited.constEnd())
        {
            if(dead_tsx_.count(dit.key().toStdString()) != 0)
            {
                const QStringList &maps = maps_by_tsx_.value(dit.key());
                int k = 0;
                while(k < maps.size())
                {
                    if(seen.insert(maps.at(k).toStdString()).second)
                        affected.append(maps.at(k));
                    k++;
                }
            }
            ++dit;
        }
    }

    // 1) Rewrite the affected maps: repoint cells to the keeper (adding its .tsx if
    //    needed) and drop any now-dead, unused <tileset> entry.
    startWriter();
    int fi = 0;
    while(fi < affected.size())
    {
        remapOneMap(affected.at(fi));
        fi++;
    }
    stopWriterAndJoin();

    // 2) Persist the tileset images (after the maps no longer reference dead ones):
    //    delete a dead tileset's .png + .tsx, otherwise save the blanked .png.
    QHash<QString, QImage>::iterator eit = edited.begin();
    while(eit != edited.end())
    {
        const QString tsx = eit.key();
        const QString png = png_by_tsx_.value(tsx);
        if(dead_tsx_.count(tsx.toStdString()) != 0)
        {
            if(!png.isEmpty() && QFile::exists(png) && !QFile::remove(png))
                qWarning() << "Unable to remove dead tileset image" << png;
            if(QFile::exists(tsx) && !QFile::remove(tsx))
                qWarning() << "Unable to remove dead tileset" << tsx;
            qDebug() << "Removed fully-transparent tileset" << tsx;
        }
        else
        {
            if(!png.isEmpty() && !eit.value().save(png, "PNG"))
                qWarning() << "Unable to write tileset image" << png;
            sheets_.insert(tsx, QPixmap::fromImage(eit.value()));  // keep memory in sync
        }
        ++eit;
    }
    if(!edited.isEmpty())
        effectiveSheets_.clear();

    // 3) Log the now-persisted merges (after maps + png are on disk).
    std::size_t li = merge_log_committed_;
    while(li < merge_log_.size())
    {
        logMerge(merge_log_[li].loser, merge_log_[li].keeper, merge_log_[li].automatic);
        li++;
    }
    merge_log_committed_ = merge_log_.size();
}

// Build (once) the canonical-tsx -> [.tmx files] index by text-scanning every map for
// its <tileset source="..."> references (cheap; no libtiled parse). Used by
// flushChanges() to rewrite only the maps that reference a merged tile.
void TileDeduplicator::buildMapIndex()
{
    map_index_built_ = true;
    maps_by_tsx_.clear();
    QStringList tmxFiles;
    {
        QDirIterator it(datapack_path_, QStringList() << QStringLiteral("*.tmx"),
                        QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext())
            tmxFiles.append(it.next());
    }
    const QString needle = QStringLiteral("source=\"");
    int fi = 0;
    while(fi < tmxFiles.size())
    {
        const QString &tmx = tmxFiles.at(fi);
        QFile f(tmx);
        if(f.open(QFile::ReadOnly | QFile::Text))
        {
            const QString text = QString::fromUtf8(f.readAll());
            f.close();
            const QString dir = QFileInfo(tmx).absolutePath();
            int pos = 0;
            while((pos = text.indexOf(needle, pos)) != -1)
            {
                const int start = pos + needle.size();
                const int end = text.indexOf(QLatin1Char('"'), start);
                if(end < 0)
                    break;
                const QString src = text.mid(start, end - start);
                pos = end + 1;
                if(src.endsWith(QStringLiteral(".tsx")))
                {
                    QString canon = QFileInfo(QDir(dir).filePath(src)).canonicalFilePath();
                    if(canon.isEmpty())
                        canon = QFileInfo(QDir(dir).filePath(src)).absoluteFilePath();
                    if(!canon.isEmpty())
                        maps_by_tsx_[canon].append(tmx);
                }
            }
        }
        fi++;
    }
}

void TileDeduplicator::startWriter()
{
    write_done_ = false;
    writer_thread_ = std::thread(&TileDeduplicator::writerLoop, this);
}

void TileDeduplicator::enqueueWrite(const QString &fileName, const QByteArray &data)
{
    {
        std::lock_guard<std::mutex> lock(write_mutex_);
        WriteJob job;
        job.fileName = fileName;
        job.data = data;
        write_queue_.push(job);
    }
    write_cv_.notify_one();
}

// Background thread: drain the queue, writing each finished map's bytes to disk.
// Exits once the queue is empty and stopWriterAndJoin() has set write_done_.
void TileDeduplicator::writerLoop()
{
    while(true)
    {
        WriteJob job;
        {
            std::unique_lock<std::mutex> lock(write_mutex_);
            while(write_queue_.empty() && !write_done_)
                write_cv_.wait(lock);
            if(write_queue_.empty())
                return;
            job = write_queue_.front();
            write_queue_.pop();
        }
        QFile f(job.fileName);
        if(f.open(QFile::WriteOnly))
        {
            f.write(job.data);
            f.close();
        }
        else
            qWarning() << "Unable to write map" << job.fileName;
    }
}

void TileDeduplicator::stopWriterAndJoin()
{
    {
        std::lock_guard<std::mutex> lock(write_mutex_);
        write_done_ = true;
    }
    write_cv_.notify_one();
    if(writer_thread_.joinable())
        writer_thread_.join();
}

void TileDeduplicator::remapOneMap(const QString &fileName)
{
    Tiled::MapReader reader;
    std::unique_ptr<Tiled::Map> map = reader.readMap(fileName);
    if(!map)
    {
        qWarning() << "Unable to read map" << fileName << ":" << reader.errorString();
        return;
    }

    ts_by_canon_.clear();
    canon_by_ts_.clear();
    map_changed_ = false;

    int ti = 0;
    while(ti < map->tilesetCount())
    {
        Tiled::SharedTileset ts = map->tilesetAt(ti);
        const QString fileName = ts->fileName();
        // Safety gate: if any referenced tileset failed to resolve (its .tsx
        // file is missing, or its image did not load), libtiled has read it as
        // an empty/short placeholder. Writing the map back then recomputes the
        // firstgids from the placeholder's (wrong) tile count, which shifts —
        // and can collide — the firstgids of every later tileset, silently
        // corrupting the map. These maps are already broken by the dangling
        // reference; refuse to touch them and tell the user to fix it first.
        if(!fileName.isEmpty() && !QFileInfo::exists(fileName))
        {
            qWarning() << "Skipping" << fileName << "- referenced tileset file is missing, map left untouched:" << fileName;
            maps_skipped_++;
            return;
        }
        if(!fileName.isEmpty() && !ts->isCollection() && ts->image().isNull())
        {
            qWarning() << "Skipping map - tileset image failed to load:" << fileName;
            maps_skipped_++;
            return;
        }
        QString canon = QFileInfo(fileName).canonicalFilePath();
        if(canon.isEmpty())
            canon = QFileInfo(fileName).absoluteFilePath();
        if(!canon.isEmpty())
        {
            ts_by_canon_.insert(canon, ts);
            canon_by_ts_.insert(ts.data(), canon);
        }
        ti++;
    }

    remapLayers(map->layers(), map.get());

    // Drop any now-dead tileset (fully transparent — every tile merged away, so no
    // cell references it any more after the repoint above). We don't use
    // map->isTilesetUsed() here: it reads a cached used-tilesets set that in-place
    // cell edits don't invalidate, so it would wrongly report the tileset as still
    // used. "dead" already guarantees it is unreferenced.
    int dti = map->tilesetCount() - 1;
    while(dti >= 0)
    {
        const QString canon = canon_by_ts_.value(map->tilesetAt(dti).data());
        if(!canon.isEmpty() && dead_tsx_.count(canon.toStdString()) != 0)
        {
            map->removeTilesetAt(dti);
            map_changed_ = true;
        }
        dti--;
    }

    if(map_changed_)
    {
        // Serialize here (Tiled is never touched off-thread), then hand the bytes
        // to the background writer: this map is final now ("no more changes into
        // it"), so its disk write overlaps the parse+remap of the next map.
        Tiled::MapWriter writer;
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        // The device-based writeMap() treats its path argument as the BASE
        // DIRECTORY (mDir = QDir(path)) for computing relative tileset/image
        // references — unlike the file-based overload. Pass the map's directory,
        // not the .tmx file path, or every reference is written one level too
        // deep (e.g. "../foo.tsx" instead of "foo.tsx"), corrupting the map.
        writer.writeMap(map.get(), &buffer, QFileInfo(fileName).absolutePath());
        buffer.close();
        if(bytes.isEmpty())
            qWarning() << "Failed to serialize map" << fileName;
        else
        {
            enqueueWrite(fileName, bytes);
            maps_rewritten_++;
        }
    }
}

void TileDeduplicator::remapLayers(const QList<Tiled::Layer *> &layers, Tiled::Map *map)
{
    int li = 0;
    while(li < layers.size())
    {
        Tiled::Layer *layer = layers.at(li);
        if(Tiled::TileLayer *tileLayer = layer->asTileLayer())
        {
            // Empty-tile clearing is safe only on visual layers; on presence
            // layers a transparent cell still blocks/triggers, so keep it.
            const bool clearable = !isPresenceLayer(tileLayer->name());
            Tiled::TileLayer::iterator it = tileLayer->begin();
            Tiled::TileLayer::iterator end = tileLayer->end();
            while(it != end)
            {
                Tiled::Cell &cell = *it;
                if(!cell.isEmpty() && cell.tileset() != nullptr)
                {
                    const QString canon = canon_by_ts_.value(cell.tileset());
                    if(!canon.isEmpty())
                    {
                        const QString key = canon + QStringLiteral("#") + QString::number(cell.tileId());
                        if(clearable && transparent_tiles_.count(key.toStdString()) != 0)
                        {
                            cell = Tiled::Cell();   // empty tile -> transparent space
                            map_changed_ = true;
                            cells_cleared_++;
                        }
                        else
                        {
                            QHash<QString, QPair<QString, int> >::const_iterator r = remap_.constFind(key);
                            if(r != remap_.constEnd())
                            {
                                Tiled::Tile *keep = keeperTileForMap(map, r.value().first, r.value().second);
                                if(keep != nullptr)
                                {
                                    cell.setTile(keep); // flip flags are preserved
                                    map_changed_ = true;
                                }
                            }
                        }
                    }
                }
                ++it;
            }
        }
        else if(Tiled::ObjectGroup *objectGroup = layer->asObjectGroup())
        {
            const QList<Tiled::MapObject *> &objects = objectGroup->objects();
            int oi = 0;
            while(oi < objects.size())
            {
                Tiled::MapObject *object = objects.at(oi);
                const Tiled::Cell &cell = object->cell();
                if(!cell.isEmpty() && cell.tileset() != nullptr)
                {
                    const QString canon = canon_by_ts_.value(cell.tileset());
                    if(!canon.isEmpty())
                    {
                        const QString key = canon + QStringLiteral("#") + QString::number(cell.tileId());
                        QHash<QString, QPair<QString, int> >::const_iterator r = remap_.constFind(key);
                        if(r != remap_.constEnd())
                        {
                            Tiled::Tile *keep = keeperTileForMap(map, r.value().first, r.value().second);
                            if(keep != nullptr)
                            {
                                Tiled::Cell newCell = cell;
                                newCell.setTile(keep);
                                object->setCell(newCell);
                                map_changed_ = true;
                            }
                        }
                    }
                }
                oi++;
            }
        }
        else if(Tiled::GroupLayer *groupLayer = layer->asGroupLayer())
            remapLayers(groupLayer->layers(), map);
        li++;
    }
}

// Return the kept tile inside this map, loading and adding its tileset if the
// map does not already reference it. The cell will then point at a tileset
// that is part of map->tilesets(), so the writer's GidMapper resolves it.
Tiled::Tile *TileDeduplicator::keeperTileForMap(Tiled::Map *map, const QString &tsx, int id)
{
    QSharedPointer<Tiled::Tileset> tileset = ts_by_canon_.value(tsx);
    if(tileset.isNull())
    {
        Tiled::MapReader reader;
        tileset = reader.readTileset(tsx);
        if(tileset.isNull())
        {
            qWarning() << "Unable to load keeper tileset" << tsx << ":" << reader.errorString();
            return nullptr;
        }
        // Standalone readTileset() leaves fileName empty, which makes the writer
        // EMBED the whole tileset (with its <image>) into the map instead of
        // writing an external <tileset source=...>. Set it so the added keeper
        // tileset stays an external reference like the datapack's others.
        tileset->setFileName(tsx);
        map->addTileset(tileset);
        ts_by_canon_.insert(tsx, tileset);
        canon_by_ts_.insert(tileset.data(), tsx);
    }
    Tiled::Tile *tile = tileset->findTile(id);
    if(tile == nullptr)
        qWarning() << "Keeper tile" << id << "missing in" << tsx;
    return tile;
}
