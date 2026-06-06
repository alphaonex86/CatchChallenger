#ifndef TILESETTAGGER_TAGMODEL_HPP
#define TILESETTAGGER_TAGMODEL_HPP

// Core (GUI-free, unit-testable) model for tagging a Tiled .tsx tileset with
// semantic categories.  Tags are stored as per-tile <property> entries in the
// .tsx itself (category / name / size).  The image is loaded only to run the
// "untagged non-transparent tile" guard the owner asked for: any tile that draws
// pixels but carries no category is reported, so nothing is forgotten.

#include <QDomDocument>
#include <QImage>
#include <QString>
#include <string>
#include <unordered_map>
#include <vector>

class TagModel {
public:
    struct TileTag {
        std::string category;   // "" => untagged
        std::string name;       // groups the tiles of one multi-tile item
        std::string size;       // e.g. "2x2"; "" if unset
    };

    TagModel();

    // Load a .tsx and its referenced image.  Returns false (and sets error()) on
    // failure.
    bool load(const QString &tsxPath);
    // Write the current tags back into the .tsx (surgical: only adds/updates the
    // category/name/size properties, preserves everything else).
    bool save();

    int tileCount() const;
    int columns() const;
    int tileWidth() const;
    int tileHeight() const;
    const QImage &image() const;

    // Tag every id in tileIds with (category,name,size).  An empty category
    // CLEARS the tag.  Marks the document dirty.
    void tagTiles(const std::vector<int> &tileIds, const std::string &category,
                  const std::string &name, const std::string &size);
    // Convenience: a rectangle (inclusive tile coords) -> the tile ids it covers.
    std::vector<int> tilesInRect(int col0, int row0, int col1, int row1) const;

    const TileTag &tagOf(int tileId) const;
    bool tileHasPixels(int tileId) const;          // any non-transparent pixel?
    std::vector<int> untaggedNonEmpty() const;     // the GUARD: pixels but no category
    std::vector<std::string> categoriesUsed() const;

    const QString &error() const;

private:
    QString tsxPath_;
    QString imagePath_;
    QDomDocument doc_;
    QImage image_;
    int tileWidth_;
    int tileHeight_;
    int tileCount_;
    int columns_;
    std::unordered_map<int, TileTag> tags_;
    TileTag emptyTag_;
    QString error_;

    QDomElement tilesetElement();
    QDomElement ensureTileElement(int id);          // find or create <tile id=...>
};

#endif // TILESETTAGGER_TAGMODEL_HPP
