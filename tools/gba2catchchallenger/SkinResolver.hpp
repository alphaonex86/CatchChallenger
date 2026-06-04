#ifndef GBA2CC_SKINRESOLVER_HPP
#define GBA2CC_SKINRESOLVER_HPP

// Maps a candidate sprite (extracted from the ROM) onto a CatchChallenger bot
// skin: reuse an existing skin when the pixels match, otherwise register a new
// one.  "Match" = both images cropped to their opaque content bounding box,
// equal cropped size, and every R/G/B/A channel within +-10% of full scale.

#include <QImage>
#include <string>
#include <vector>

class SkinResolver {
public:
    // skinBotDir is "<datapack>/skin/bot".  perChannelTolerance is the absolute
    // 0..255 difference allowed per channel (10% of 255 ~= 26).
    SkinResolver(const std::string &skinBotDir, int perChannelTolerance);

    // Load the existing skins so they can be reused.
    void loadExisting();

    // Return the skin folder name for the candidate, reusing an existing skin
    // on a pixel match or creating a new "skin/bot/<n>/front.png".  Returns an
    // empty string only when the candidate image is itself empty.
    std::string resolve(const QImage &candidate);

    size_t addedCount() const;
    size_t reuseCount() const;

private:
    struct Entry {
        std::string name;
        QImage cropped; // ARGB32, cropped to opaque content
    };
    static QImage cropToContent(const QImage &src);
    bool sameImage(const QImage &a, const QImage &b) const;

    std::string skinBotDir_;
    int tolerance_;
    int nextNewId_;
    std::vector<Entry> entries_;
    size_t added_;
    size_t reused_;
};

#endif // GBA2CC_SKINRESOLVER_HPP
