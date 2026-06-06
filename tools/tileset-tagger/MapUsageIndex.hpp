#ifndef TILESETTAGGER_MAPUSAGEINDEX_HPP
#define TILESETTAGGER_MAPUSAGEINDEX_HPP

// Finds WHERE the tiles of a tileset are used across the datapack maps, and
// renders those maps — so the tagger can show a selected tile group IN SITU
// (step 2 of the owner's two-step tagging: select a rectangle of tiles, then see
// every place it appears on the real maps, marked with animated rectangles).
//
// Uses the vendored libtiled (catchchallenger_tiled) to decode .tmx layers
// (base64 + zstd/zlib/csv) and resolve gids through their tilesets.  GUI-free so
// it can be unit-tested with --usage.

#include <QImage>
#include <QPoint>
#include <QString>
#include <map>
#include <memory>
#include <vector>

namespace Tiled { class Map; }

class MapUsageIndex {
public:
    struct Usage {
        QString mapPath;
        QString mapLabel;             // short label for the combo
        int mapW;                     // map size, in tiles
        int mapH;
        int tileW;                    // tile size, in pixels
        int tileH;
        std::vector<QPoint> cells;    // tile coords using a selected tile
    };

    MapUsageIndex();
    ~MapUsageIndex();

    // Scan the datapack for .tmx that reference this .tsx (cheap text pre-filter
    // by basename; the real per-tileset match happens in findUsages).
    void build(const QString &tsxPath);
    int candidateCount() const;

    // For these tileset tile ids, the maps + cells that use them, sorted by
    // descending usage count.
    std::vector<Usage> findUsages(const std::vector<int> &tileIds);

    // Composite-render a map's tile layers to an image (cached).
    QImage render(const QString &mapPath);

    const QString &error() const;

private:
    QString tsxCanonical_;
    std::vector<QString> candidates_;
    std::map<QString,std::shared_ptr<Tiled::Map> > loaded_;
    std::map<QString,QImage> rendered_;
    QString error_;
    Tiled::Map *mapFor(const QString &path);   // load + cache
};

#endif // TILESETTAGGER_MAPUSAGEINDEX_HPP
