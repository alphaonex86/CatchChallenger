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
#include <unordered_set>
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

    // Load a .tsx (read-only: dimensions + image + animation hint) and its tags
    // from the sidecar.  Returns false (and sets error()) on failure.
    bool load(const QString &tsxPath);
    // Write the current tags to the SIDECAR json (out of the datapack — the .tsx
    // is never modified).
    bool save();
    const QString &tagFilePath() const;   // sidecar tag file for this tileset
    const QString &sidecarDir() const;    // datapack sidecar directory
    const QString &tsxPath() const;       // the loaded .tsx path (for the window title)

    int tileCount() const;
    int columns() const;
    int tileWidth() const;
    int tileHeight() const;
    const QImage &image() const;

    // Tag every id in tileIds with a category + attributes (name, size,
    // horizontalRepeat, ...).  An empty category CLEARS the tag.
    void tagTiles(const std::vector<int> &tileIds, const std::string &category,
                  const std::map<std::string,std::string> &attributes);
    // Accept the auto-guesses on these tiles as correct: drop the `auto=guess`
    // flag (yellow -> verified) without changing the category.
    void markVerified(const std::vector<int> &tileIds);
    void markAllVerified();   // confirm EVERY tag (drop auto=guess) — the review "save all"

    // Review progression over the whole tileset.
    struct Counts { int verified; int toReview; int untagged; };
    Counts progress() const;
    // Convenience: a rectangle (inclusive tile coords) -> the tile ids it covers.
    std::vector<int> tilesInRect(int col0, int row0, int col1, int row1) const;

    // SIMILAR-GROUP links (in-memory only — NOT written to the tileset, NOT a tag
    // merge): two tiles are the SAME OBJECT on a different background when >=70% of
    // their pixels match over the OVERLAP (where both have content).  E.g. an object
    // baked on a terrain and the same object on a transparent background.  Useful to
    // reason about map<->tileset relations later.
    struct Similar { int tileId; int percent; };
    const std::vector<Similar> &similarTo(int tileId) const;   // others >=70% same, desc; [] if none
    int similarCount() const;                                  // tiles that have at least one link

    const TileTag &tagOf(int tileId) const;
    bool tileHasPixels(int tileId) const;          // any non-transparent pixel?
    bool tileAnimated(int tileId) const;           // carries an animation property?
    bool tileGreenish(int tileId) const;           // green-dominant art (vegetation hint)
    std::vector<int> untaggedNonEmpty() const;     // the GUARD: pixels but no category
    std::vector<int> unverifiedTiles() const;      // untagged-with-pixels OR auto=guess, in id order
    std::vector<std::string> categoriesUsed() const;

    const QString &error() const;

private:
    QString tsxPath_;
    QString imagePath_;
    QString sidecarDir_;
    QString tagFilePath_;
    QDomDocument doc_;
    QImage image_;
    int tileWidth_;
    int tileHeight_;
    int tileCount_;
    int columns_;
    std::unordered_map<int, TileTag> tags_;
    std::unordered_set<int> animatedTiles_;
    std::unordered_map<int, std::vector<Similar> > similar_;
    TileTag emptyTag_;
    QString error_;

    QDomElement tilesetElement();
    void loadSidecarTags();                          // read tags_ from tagFilePath_
    void detectSimilarGroups();                      // in-memory same-object-different-bg links
};

#endif // TILESETTAGGER_TAGMODEL_HPP
