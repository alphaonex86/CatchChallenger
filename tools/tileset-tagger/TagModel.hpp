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
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

class TagModel {
public:
    // A tile's tag: a primary CATEGORY (table, tree-canopy, building-wall, ...)
    // plus free-form ATTRIBUTES — name (groups the tiles of one multi-tile item),
    // size ("2x2"), and behaviour flags like horizontalRepeat / verticalRepeat
    // (a fence tiles sideways, water repeats both ways).  Every attribute is
    // stored as its own <property> in the .tsx, so new ones cost nothing.
    struct TileTag {
        std::string category;                          // "" => untagged
        std::map<std::string,std::string> attributes;  // name, size, horizontalRepeat, ...
        std::string attr(const std::string &key) const
        {
            std::map<std::string,std::string>::const_iterator it=attributes.find(key);
            return it==attributes.cend() ? std::string() : it->second;
        }
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

    // Tag every id in tileIds with a category + attributes (name, size,
    // horizontalRepeat, ...).  An empty category CLEARS the tag.
    void tagTiles(const std::vector<int> &tileIds, const std::string &category,
                  const std::map<std::string,std::string> &attributes);
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
    // remove the <property> entries this tool manages (category + the known
    // attribute vocabulary + any key in extraKeys) so save() can rewrite them
    // cleanly while leaving foreign engine/Tiled properties intact.
    void stripManagedProperties(QDomElement props, const std::map<std::string,std::string> *extraKeys);
};

#endif // TILESETTAGGER_TAGMODEL_HPP
