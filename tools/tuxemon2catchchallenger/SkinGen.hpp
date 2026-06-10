#ifndef TUXEMON2CC_SKINGEN_HPP
#define TUXEMON2CC_SKINGEN_HPP

// Generates CatchChallenger skins from Tuxemon character sprites.  A skin is a
// folder skin/<category>/<name>/ holding front.png + back.png (80x80 battle),
// trainer.png (48x96 overworld walk-sheet, 3 frames x 4 directions of 16x24) and
// swim.png.  Tuxemon overworld sprites (sprites/<name>.png) and battle sheets
// (gfx/sprites/player/<sheet>.png) have a different, per-template layout, so we
// extract a representative frame and build valid-dimension PNGs from it (the
// engine only needs the files to exist and be the right size).

#include <string>
#include <unordered_set>

namespace tuxemon {

class SkinGen {
public:
    SkinGen(const std::string &modRoot, const std::string &outRoot);
    // category = "fighter" or "bot".  overworldSprite/battleSheet are Tuxemon
    // base names (fallback to "adventurer" when missing).  Idempotent.
    // Returns the skin name actually written (== skinName).
    bool ensure(const std::string &category, const std::string &skinName,
                const std::string &overworldSprite, const std::string &battleSheet);
    int count() const { return generated_; }

private:
    std::string modRoot_;
    std::string outRoot_;
    std::unordered_set<std::string> done_;
    int generated_;
};

} // namespace tuxemon

#endif // TUXEMON2CC_SKINGEN_HPP
