#include "TileDeduplicator.hpp"

#include <QLabel>
#include <QPushButton>
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
#include <QSizePolicy>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QBuffer>
#include <QFile>
#include <QCoreApplication>
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

#include <cstring>

// All tiles in this datapack are 16x16; the spec fixes the size.
static const int kTileW = 16;
static const int kTileH = 16;
static const int kTilePx = kTileW * kTileH;               // 256

// A pair is SIMILAR when at least 90% of pixels match. Below that it is
// DIFFERENT. kSimilarMin is the smallest pixel count that reaches 90%
// (ceil(0.9*256)=231); once more than (kTilePx-kSimilarMin) pixels mismatch a
// pair can no longer be SIMILAR, so comparePixels() early-exits there.
static const int kSimilarMin = (kTilePx * 90 + 99) / 100; // 231
static const int kMaxMismatch = kTilePx - kSimilarMin;    // 25

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

TileDeduplicator::TileDeduplicator(const QString &datapackPath, bool batch, bool checkAll, QWidget *parent) :
    QWidget(parent),
    datapack_path_(datapackPath),
    batch_(batch),
    check_all_(checkAll),
    i_(0),
    j_(1),
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
    preview_a_(new QLabel(decision_widget_)),
    preview_b_(new QLabel(decision_widget_)),
    info_a_(new QLabel(decision_widget_)),
    info_b_(new QLabel(decision_widget_)),
    skip_button_(new QPushButton(tr("SKIP"), decision_widget_)),
    back_button_(new QPushButton(tr("BACK"), decision_widget_)),
    save_button_(new QPushButton(tr("SAVE NOW"), decision_widget_)),
    keep_left_button_(new QPushButton(tr("\342\227\200 KEEP LEFT"), decision_widget_)),
    keep_right_button_(new QPushButton(tr("KEEP RIGHT \342\226\266"), decision_widget_)),
    scroll_a_(new QScrollArea(decision_widget_)),
    scroll_b_(new QScrollArea(decision_widget_)),
    tileset_a_(new QLabel()),
    tileset_b_(new QLabel()),
    anim_timer_(new QTimer(this)),
    anim_on_(true),
    pick_left_(-1),
    pick_right_(-1),
    dirty_(false)
{
    setWindowTitle(tr("CatchChallenger - Tileset tile deduplicator"));

    QVBoxLayout *root = new QVBoxLayout(this);
    status_label_->setText(tr("Loading tilesets..."));
    root->addWidget(status_label_);

    prompt_label_->setText(tr("SIMILAR tiles. Click the big tile to KEEP it (the other is "
                              "dropped, all maps repointed to it); SKIP keeps both. Manual: "
                              "click a tile in each tileset below, then KEEP LEFT or KEEP "
                              "RIGHT to choose which one survives."));
    prompt_label_->setWordWrap(true);

    QHBoxLayout *previews = new QHBoxLayout();
    QVBoxLayout *colA = new QVBoxLayout();
    QVBoxLayout *colB = new QVBoxLayout();
    // Previews grow when the window is maximized (re-rendered crisp in
    // resizeEvent); clicking either one chooses it as the keeper.
    preview_a_->setMinimumSize(kTileW * kZoom, kTileH * kZoom);
    preview_a_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    preview_a_->setAlignment(Qt::AlignCenter);
    preview_a_->setCursor(Qt::PointingHandCursor);
    preview_a_->installEventFilter(this);
    preview_b_->setMinimumSize(kTileW * kZoom, kTileH * kZoom);
    preview_b_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    preview_b_->setAlignment(Qt::AlignCenter);
    preview_b_->setCursor(Qt::PointingHandCursor);
    preview_b_->installEventFilter(this);
    // Each column also shows the tile's full tileset at 1x (scrollable), with a
    // blinking marker at the tile, set up in showPrompt()/onAnimTick().
    scroll_a_->setWidget(tileset_a_);
    scroll_b_->setWidget(tileset_b_);
    scroll_a_->setMinimumSize(360, 300);
    scroll_b_->setMinimumSize(360, 300);
    scroll_a_->setAlignment(Qt::AlignCenter);
    scroll_b_->setAlignment(Qt::AlignCenter);
    // Clicking a tile in a sheet picks that tileset's candidate; KEEP LEFT / KEEP
    // RIGHT then chooses which one survives — a manual replace the auto-compare missed.
    tileset_a_->installEventFilter(this);
    tileset_b_->installEventFilter(this);
    tileset_a_->setCursor(Qt::PointingHandCursor);
    tileset_b_->setCursor(Qt::PointingHandCursor);
    colA->addWidget(preview_a_);
    colA->addWidget(info_a_);
    colA->addWidget(scroll_a_);
    colB->addWidget(preview_b_);
    colB->addWidget(info_b_);
    colB->addWidget(scroll_b_);
    previews->addLayout(colA);
    previews->addLayout(colB);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addWidget(back_button_);
    buttons->addWidget(skip_button_);
    buttons->addWidget(save_button_);
    buttons->addWidget(keep_left_button_);
    buttons->addWidget(keep_right_button_);

    QVBoxLayout *decisionLayout = new QVBoxLayout(decision_widget_);
    decisionLayout->addWidget(prompt_label_);
    decisionLayout->addLayout(previews);
    decisionLayout->addLayout(buttons);
    root->addWidget(decision_widget_);
    decision_widget_->hide();

    connect(skip_button_, &QPushButton::clicked, this, &TileDeduplicator::onSkipClicked);
    connect(back_button_, &QPushButton::clicked, this, &TileDeduplicator::onBackClicked);
    connect(save_button_, &QPushButton::clicked, this, &TileDeduplicator::onSaveClicked);
    connect(keep_left_button_, &QPushButton::clicked, this, &TileDeduplicator::onKeepLeftPick);
    connect(keep_right_button_, &QPushButton::clicked, this, &TileDeduplicator::onKeepRightPick);

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
}

void TileDeduplicator::loadAndStart()
{
    if(!loadTilesets())
    {
        status_label_->setText(tr("No 16x16 tileset found under: %1").arg(datapack_path_));
        return;
    }
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
            }

            const QList<Tiled::Tile *> &tiles = tileset->tiles();
            int ti = 0;
            while(ti < tiles.size())
            {
                Tiled::Tile *tile = tiles.at(ti);
                if(tile->isAnimated())
                {
                    // keep animated tiles untouched (merging drops their frames)
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
    // Exact-byte fast path (guard against the rare hash collision).
    if(a.hash == b.hash &&
       std::memcmp(a.px.data(), b.px.data(), kTilePx * sizeof(QRgb)) == 0)
        return Identical;

    int mismatch = 0;
    bool allMatch = true;
    int k = 0;
    while(k < kTilePx)
    {
        const QRgb pa = a.px[k];
        const QRgb pb = b.px[k];
        const int aa = qAlpha(pa);
        const int ab = qAlpha(pb);

        bool pixelMatch;
        if(!within15(aa, ab))
            pixelMatch = false;
        else if(aa <= kAlphaTransparent && ab <= kAlphaTransparent)
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
    return Similar;
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
    merge_log_.push_back(loser);
    dirty_ = true;
    effectiveSheets_.clear();  // merge changed: rebuild the "with changes" sheets
    if(sourceLost != nullptr)
        *sourceLost = (loser == a);
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
            if(batch_ || skipped_sources_.count(tiles_[i_].hash) != 0)
                j_++; // batch run, or user already chose to keep this source tile
            else
            {
                showPrompt(i_, j_);
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

void TileDeduplicator::showPrompt(int a, int b)
{
    step_timer_->stop();
    const int z = previewZoom();
    preview_a_->setPixmap(renderPreview(tiles_[a], z));
    preview_b_->setPixmap(renderPreview(tiles_[b], z));
    info_a_->setText(tr("%1\nid %2  alpha %3")
                     .arg(QFileInfo(tiles_[a].tsx).fileName())
                     .arg(tiles_[a].id)
                     .arg(tiles_[a].alpha_sum));
    info_b_->setText(tr("%1\nid %2  alpha %3")
                     .arg(QFileInfo(tiles_[b].tsx).fileName())
                     .arg(tiles_[b].id)
                     .arg(tiles_[b].alpha_sum));
    anim_on_ = true;
    pick_left_ = -1;
    pick_right_ = -1;
    keep_left_button_->setEnabled(false);
    keep_right_button_->setEnabled(false);
    renderTilesetView(tileset_a_, scroll_a_, tiles_[a], anim_on_, pick_left_);
    renderTilesetView(tileset_b_, scroll_b_, tiles_[b], anim_on_, pick_right_);
    anim_timer_->start();
    back_button_->setEnabled(!undo_.empty());
    updateSaveButton();
    decision_widget_->show();
}

void TileDeduplicator::onKeepA()
{
    mergeKeeping(i_, j_);  // keep the left tile (i_), drop the right (j_)
}

void TileDeduplicator::onKeepB()
{
    mergeKeeping(j_, i_);  // keep the right tile (j_), drop the left (i_)
}

// For a SIMILAR pair the user clicked the tile to keep: the loser points at the
// keeper and every .tmx will be repointed to it.
void TileDeduplicator::mergeKeeping(int keeper, int loser)
{
    pushUndo(0);           // record pre-decision state so BACK can revert it
    tiles_[loser].merged_into = keeper;
    merge_log_.push_back(loser);
    similar_merges_++;
    dirty_ = true;
    effectiveSheets_.clear();  // merge changed: rebuild the "with changes" sheets
    decision_widget_->hide();
    anim_timer_->stop();
    if(loser == i_)
        advanceSource();   // the outer/source tile was dropped
    else
        j_++;              // the inner tile was dropped, keep scanning this source
    step_timer_->start();
}

// Magnification for the previews from their current on-screen size, so they grow
// when the window is maximized; never below the base kZoom.
int TileDeduplicator::previewZoom() const
{
    int s = preview_a_->width();
    if(preview_a_->height() < s)
        s = preview_a_->height();
    int z = s / kTileW;
    if(z < kZoom)
        z = kZoom;
    return z;
}

bool TileDeduplicator::eventFilter(QObject *watched, QEvent *event)
{
    if(event->type() == QEvent::MouseButtonPress && decision_widget_->isVisible())
    {
        if(watched == preview_a_)
        {
            onKeepA();
            return true;
        }
        if(watched == preview_b_)
        {
            onKeepB();
            return true;
        }
        if(watched == tileset_a_)
        {
            onSheetClick(true, static_cast<QMouseEvent *>(event)->position().toPoint());
            return true;
        }
        if(watched == tileset_b_)
        {
            onSheetClick(false, static_cast<QMouseEvent *>(event)->position().toPoint());
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void TileDeduplicator::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // Re-render the previews crisp at the new size while a SIMILAR pair is shown.
    if(decision_widget_->isVisible() &&
       i_ < static_cast<int>(tiles_.size()) && j_ < static_cast<int>(tiles_.size()))
    {
        const int z = previewZoom();
        preview_a_->setPixmap(renderPreview(tiles_[i_], z));
        preview_b_->setPixmap(renderPreview(tiles_[j_], z));
    }
}

void TileDeduplicator::onSkipClicked()
{
    pushUndo(0);
    // Remember this source tile so we never ask about it again this run. Record
    // the hash in the undo step only if WE added it, so BACK removes exactly that.
    if(skipped_sources_.insert(tiles_[i_].hash).second)
        undo_.back().skipHashAdded = tiles_[i_].hash;
    skips_++;
    decision_widget_->hide();
    anim_timer_->stop();
    j_++;
    step_timer_->start();
}

void TileDeduplicator::pushUndo(quint64 skipHash)
{
    UndoStep u;
    u.i = i_;
    u.j = j_;
    u.mergeLogLen = merge_log_.size();
    u.identical = identical_merges_;
    u.similar = similar_merges_;
    u.skips = skips_;
    u.skipHashAdded = skipHash;
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
    while(merge_log_.size() > u.mergeLogLen)
    {
        const int idx = merge_log_.back();
        merge_log_.pop_back();
        tiles_[idx].merged_into = -1;
    }
    if(u.skipHashAdded != 0)
        skipped_sources_.erase(u.skipHashAdded);
    identical_merges_ = u.identical;
    similar_merges_ = u.similar;
    skips_ = u.skips;
    dirty_ = true;
    effectiveSheets_.clear();  // merges reverted: rebuild the "with changes" sheets
    i_ = u.i;
    j_ = u.j;
    showPrompt(i_, j_);
}

// Persist the current merges/skips to disk now, without ending the run, so the
// user need not finish every prompt to save progress.
void TileDeduplicator::onSaveClicked()
{
    save_button_->setEnabled(false);
    status_label_->setText(tr("Saving..."));
    status_label_->repaint();
    applyRemaps();             // writes pending maps and clears dirty_
    status_label_->setText(tr("Saved. processing... %1/%2 tiles")
                           .arg(i_ + 1).arg(static_cast<int>(tiles_.size())));
    updateSaveButton();        // dirty_ is now false -> SAVE stays disabled
}

void TileDeduplicator::onAnimTick()
{
    if(!decision_widget_->isVisible() ||
       i_ >= static_cast<int>(tiles_.size()) || j_ >= static_cast<int>(tiles_.size()))
        return;
    anim_on_ = !anim_on_;
    renderTilesetView(tileset_a_, scroll_a_, tiles_[i_], anim_on_, pick_left_);
    renderTilesetView(tileset_b_, scroll_b_, tiles_[j_], anim_on_, pick_right_);
}

// Build (and cache) the tileset sheet "with the changes done": start from the
// on-disk sheet, then repaint every tile that has been merged away this session
// with its keeper tile's pixels, so the prompt previews the deduplicated result.
// Purely in-memory — the .tsx/.png on disk is never touched; the cache is cleared
// on every merge and discarded at exit, so a fresh run re-reads the clean tilesets.
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

// A click on a tileset view picks a tile: the left sheet's pick is the manual
// replacement SOURCE, the right sheet's the DESTINATION (a green marker shows it).
void TileDeduplicator::onSheetClick(bool leftSheet, const QPoint &pos)
{
    if(i_ >= static_cast<int>(tiles_.size()) || j_ >= static_cast<int>(tiles_.size()))
        return;
    const QString tsx = leftSheet ? tiles_[i_].tsx : tiles_[j_].tsx;
    const int cols = sheetCols_.value(tsx, 1);
    if(cols <= 0 || pos.x() < 0 || pos.y() < 0)
        return;
    const int tileId = (pos.y() / kTileH) * cols + (pos.x() / kTileW);
    const int idx = tile_index_.value(qMakePair(tsx, tileId), -1);
    // Not selectable: an empty/animated/absent tile (no index), or a tile already
    // merged away this session — it now shows as empty space, so picking it is
    // meaningless (no pixels left, and it can't be a keeper or a loser).
    if(idx < 0 || tiles_[idx].merged_into != -1)
        return;
    if(leftSheet)
    {
        pick_left_ = idx;
        renderTilesetView(tileset_a_, scroll_a_, tiles_[i_], anim_on_, pick_left_);
    }
    else
    {
        pick_right_ = idx;
        renderTilesetView(tileset_b_, scroll_b_, tiles_[j_], anim_on_, pick_right_);
    }
    const bool ready = (pick_left_ >= 0 && pick_right_ >= 0 && pick_left_ != pick_right_);
    keep_left_button_->setEnabled(ready);
    keep_right_button_->setEnabled(ready);
}

// Commit the manually-picked source->dest replacement (a forced merge the
// auto-compare did not make), revertible with BACK, then clear the picks.
void TileDeduplicator::onKeepLeftPick()
{
    manualReplace(pick_left_, pick_right_);   // keep the LEFT pick, replace the right with it
}

void TileDeduplicator::onKeepRightPick()
{
    manualReplace(pick_right_, pick_left_);   // keep the RIGHT pick, replace the left with it
}

// Commit a manual replacement: every map reference to the loser tile is rewritten
// to the keeper tile. Revertible with BACK; clears the picks afterwards.
void TileDeduplicator::manualReplace(int keeper, int loser)
{
    if(keeper < 0 || loser < 0 || keeper == loser)
        return;
    if(tiles_[loser].merged_into != -1)
        return;            // loser already merged away
    pushUndo(0);
    tiles_[loser].merged_into = keeper;
    merge_log_.push_back(loser);
    similar_merges_++;
    dirty_ = true;
    effectiveSheets_.clear();  // merge changed: the sheets below must show it now
    pick_left_ = -1;
    pick_right_ = -1;
    keep_left_button_->setEnabled(false);
    keep_right_button_->setEnabled(false);
    updateSaveButton();
    renderTilesetView(tileset_a_, scroll_a_, tiles_[i_], anim_on_, pick_left_);
    renderTilesetView(tileset_b_, scroll_b_, tiles_[j_], anim_on_, pick_right_);
}

void TileDeduplicator::updateSaveButton()
{
    save_button_->setEnabled(dirty_);
}

void TileDeduplicator::finish()
{
    step_timer_->stop();
    anim_timer_->stop();
    decision_widget_->hide();
    status_label_->setText(tr("Rewriting maps..."));
    status_label_->repaint();
    QCoreApplication::processEvents();

    applyRemaps();

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
    {
        dirty_ = false;
        return; // nothing merged and no empty tiles to clear: leave maps untouched
    }

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
    dirty_ = false;
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

    if(map_changed_)
    {
        // Serialize here (Tiled is never touched off-thread), then hand the bytes
        // to the background writer: this map is final now ("no more changes into
        // it"), so its disk write overlaps the parse+remap of the next map.
        Tiled::MapWriter writer;
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        writer.writeMap(map.get(), &buffer, fileName);
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
