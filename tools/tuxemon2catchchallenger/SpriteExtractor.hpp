#ifndef TUXEMON2CC_SPRITEEXTRACTOR_HPP
#define TUXEMON2CC_SPRITEEXTRACTOR_HPP

// Turns a Tuxemon battle sprite-sheet ({slug}-sheet.png) into the three PNGs
// CatchChallenger expects in monsters/<id>/ : front.png, back.png, small.png.
//
// The sheet is a COMBINED sprite sheet (tuxemon/db.py MonsterSpritesModel):
//   front = (0,0,64,64), back = (64,0,64,64),
//   menu1 = (0,64,24,24), menu2 = (24,64,24,24)
// with optional per-monster rect overrides in the monster YAML.  front/back
// are real distinct poses (back is NOT a mirror); menu1 becomes small.png.

#include <string>

namespace tuxemon {

// One region of the combined sheet, in pixels.
struct SheetRect {
    int x;
    int y;
    int w;
    int h;
    SheetRect();
    SheetRect(int x_, int y_, int w_, int h_);
};

class SpriteExtractor {
public:
    // sheetPath -> writes front/back/small.png into outDir (created if needed).
    // Returns false when the sheet cannot be read.
    static bool extract(const std::string &sheetPath, const std::string &outDir,
                        const SheetRect &front, const SheetRect &back,
                        const SheetRect &menu1);
};

} // namespace tuxemon

#endif // TUXEMON2CC_SPRITEEXTRACTOR_HPP
