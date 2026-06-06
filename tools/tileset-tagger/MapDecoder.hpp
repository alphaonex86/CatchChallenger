#ifndef TILESETTAGGER_MAPDECODER_HPP
#define TILESETTAGGER_MAPDECODER_HPP

// Decode a .tmx map THROUGH the tags into a tileset-agnostic CATEGORY grid, and
// render it — each cell drawn as its category colour — next to the real map, so a
// human can SEE whether the tagging is meaningful (grass green, buildings one hue,
// water blue, …). The first step of the learn-from-tags generator.
//
// A map uses several tilesets; each cell's gid resolves to (tileset, tileId), and
// the category comes from that tileset's sidecar tags (one TagModel per tileset).

#include <QColor>
#include <QImage>
#include <QString>
#include <string>
#include <vector>

class MapDecoder {
public:
    struct Result {
        int w;
        int h;
        int tileW;
        int tileH;
        std::vector<std::string> categoryGrid;   // size w*h; "" = no category
        QImage realRender;                        // the map as drawn
        QImage categoryRender;                    // each cell = its category colour
        int totalDrawn;                           // cells with any tile
        int tagged;                               // cells whose top tile is tagged
        int untagged;                             // cells with a tile but no tag
    };
    // Returns false + sets error on failure.
    bool decode(const QString &mapPath, Result &out, QString &error);

    // semantic colour for a category (grass green, water blue, …); shared with
    // the generator's render. Unknown categories get a stable hash colour.
    static QColor categoryColor(const std::string &cat);
};

#endif // TILESETTAGGER_MAPDECODER_HPP
