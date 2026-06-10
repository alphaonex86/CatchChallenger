#ifndef TUXEMON2CC_SPRITEEXTRACTOR_HPP
#define TUXEMON2CC_SPRITEEXTRACTOR_HPP

// Turns a Tuxemon battle sprite-sheet ({slug}-sheet.png) into the three PNGs
// CatchChallenger expects in monsters/<id>/ : front.png, back.png, small.png.
//
// Tuxemon battle sheets are 2-column (the same pose twice for the idle bob);
// the left column is taken as the front pose, then alpha-cropped.  Tuxemon has
// no dedicated back sprite, so back.png is the front mirrored horizontally;
// small.png is the front scaled down to an icon.

#include <string>

namespace tuxemon {

class SpriteExtractor {
public:
    // sheetPath -> writes front/back/small.png into outDir (created if needed).
    // Returns false when the sheet cannot be read.
    static bool extract(const std::string &sheetPath, const std::string &outDir);
};

} // namespace tuxemon

#endif // TUXEMON2CC_SPRITEEXTRACTOR_HPP
