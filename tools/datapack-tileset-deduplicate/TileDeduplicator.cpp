#include "TileDeduplicator.hpp"

#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QColor>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
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

// Per-channel "within +-10%" test, matching the project's image checker
// (testingmap2png.py): the difference must be within 10% of the larger value,
// and a 0 requires an exact 0 on the other side. Integer form of
// |x-y| <= 0.10*max(x,y).
static inline bool within10(int x, int y)
{
    const int d = (x > y) ? (x - y) : (y - x);
    const int m = (x > y) ? x : y;
    return d * 10 <= m;
}

TileDeduplicator::TileDeduplicator(const QString &datapackPath, bool batch, QWidget *parent) :
    QWidget(parent),
    datapack_path_(datapackPath),
    batch_(batch),
    i_(0),
    j_(1),
    map_changed_(false),
    identical_merges_(0),
    similar_merges_(0),
    skips_(0),
    maps_rewritten_(0),
    maps_skipped_(0),
    step_timer_(new QTimer(this)),
    status_label_(new QLabel(this)),
    decision_widget_(new QWidget(this)),
    prompt_label_(new QLabel(this)),
    preview_a_(new QLabel(decision_widget_)),
    preview_b_(new QLabel(decision_widget_)),
    info_a_(new QLabel(decision_widget_)),
    info_b_(new QLabel(decision_widget_)),
    skip_button_(new QPushButton(tr("SKIP"), decision_widget_)),
    merge_button_(new QPushButton(tr("MERGE"), decision_widget_))
{
    setWindowTitle(tr("CatchChallenger - Tileset tile deduplicator"));

    QVBoxLayout *root = new QVBoxLayout(this);
    status_label_->setText(tr("Loading tilesets..."));
    root->addWidget(status_label_);

    prompt_label_->setText(tr("These two tiles are SIMILAR. Merge them into one, or keep both?"));

    QHBoxLayout *previews = new QHBoxLayout();
    QVBoxLayout *colA = new QVBoxLayout();
    QVBoxLayout *colB = new QVBoxLayout();
    preview_a_->setFixedSize(kTileW * kZoom, kTileH * kZoom);
    preview_b_->setFixedSize(kTileW * kZoom, kTileH * kZoom);
    colA->addWidget(preview_a_);
    colA->addWidget(info_a_);
    colB->addWidget(preview_b_);
    colB->addWidget(info_b_);
    previews->addLayout(colA);
    previews->addLayout(colB);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addWidget(skip_button_);
    buttons->addWidget(merge_button_);

    QVBoxLayout *decisionLayout = new QVBoxLayout(decision_widget_);
    decisionLayout->addWidget(prompt_label_);
    decisionLayout->addLayout(previews);
    decisionLayout->addLayout(buttons);
    root->addWidget(decision_widget_);
    decision_widget_->hide();

    connect(skip_button_, &QPushButton::clicked, this, &TileDeduplicator::onSkipClicked);
    connect(merge_button_, &QPushButton::clicked, this, &TileDeduplicator::onMergeClicked);

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
// non-empty ones. Animated tiles and tiles carrying properties are skipped:
// merging them away would silently drop their animation/metadata.
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

            const QList<Tiled::Tile *> &tiles = tileset->tiles();
            int ti = 0;
            while(ti < tiles.size())
            {
                Tiled::Tile *tile = tiles.at(ti);
                if(tile->isAnimated() || !tile->properties().isEmpty())
                {
                    // keep animation/property-bearing tiles untouched
                }
                else
                {
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
                            tiles_.push_back(std::move(entry));
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
        if(!within10(aa, ab))
            pixelMatch = false;
        else if(aa <= kAlphaTransparent && ab <= kAlphaTransparent)
            pixelMatch = true; // both transparent: RGB is not rendered
        else
            pixelMatch = within10(qRed(pa), qRed(pb)) &&
                         within10(qGreen(pa), qGreen(pb)) &&
                         within10(qBlue(pa), qBlue(pb));

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

        const CompareLevel level = comparePixels(tiles_[i_], tiles_[j_]);
        if(level == Identical)
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

QPixmap TileDeduplicator::renderPreview(const TileEntry &t) const
{
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

    const int w = kTileW * kZoom;
    const int h = kTileH * kZoom;
    QPixmap canvas(w, h);
    QPainter painter(&canvas);

    // Checkerboard background so transparent parts of the tile are visible.
    const QColor light(200, 200, 200);
    const QColor dark(120, 120, 120);
    int cy = 0;
    while(cy < kTileH)
    {
        int cx = 0;
        while(cx < kTileW)
        {
            const bool even = ((cx + cy) & 1) == 0;
            painter.fillRect(cx * kZoom, cy * kZoom, kZoom, kZoom, even ? light : dark);
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
    preview_a_->setPixmap(renderPreview(tiles_[a]));
    preview_b_->setPixmap(renderPreview(tiles_[b]));
    info_a_->setText(tr("%1\nid %2  alpha %3")
                     .arg(QFileInfo(tiles_[a].tsx).fileName())
                     .arg(tiles_[a].id)
                     .arg(tiles_[a].alpha_sum));
    info_b_->setText(tr("%1\nid %2  alpha %3")
                     .arg(QFileInfo(tiles_[b].tsx).fileName())
                     .arg(tiles_[b].id)
                     .arg(tiles_[b].alpha_sum));
    decision_widget_->show();
}

void TileDeduplicator::onMergeClicked()
{
    bool sourceLost = false;
    mergePair(i_, j_, &sourceLost);
    similar_merges_++;
    decision_widget_->hide();
    if(sourceLost)
        advanceSource();
    else
        j_++;
    step_timer_->start();
}

void TileDeduplicator::onSkipClicked()
{
    // Remember this source tile so we never ask about it again this run.
    skipped_sources_.insert(tiles_[i_].hash);
    skips_++;
    decision_widget_->hide();
    j_++;
    step_timer_->start();
}

void TileDeduplicator::finish()
{
    step_timer_->stop();
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
             << "maps-skipped(unresolved tileset):" << maps_skipped_;
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

    if(remap_.isEmpty())
        return; // nothing merged, leave every map untouched

    QStringList tmxFiles;
    {
        QDirIterator it(datapack_path_, QStringList() << QStringLiteral("*.tmx"),
                        QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext())
            tmxFiles.append(it.next());
    }
    tmxFiles.sort();

    int fi = 0;
    while(fi < tmxFiles.size())
    {
        remapOneMap(tmxFiles.at(fi));
        fi++;
    }
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
        Tiled::MapWriter writer;
        if(!writer.writeMap(map.get(), fileName))
            qWarning() << "Unable to write map" << fileName << ":" << writer.errorString();
        else
            maps_rewritten_++;
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
        map->addTileset(tileset);
        ts_by_canon_.insert(tsx, tileset);
        canon_by_ts_.insert(tileset.data(), tsx);
    }
    Tiled::Tile *tile = tileset->findTile(id);
    if(tile == nullptr)
        qWarning() << "Keeper tile" << id << "missing in" << tsx;
    return tile;
}
