#ifndef TUXEMON2CC_WORLDWRITER_HPP
#define TUXEMON2CC_WORLDWRITER_HPP

// Writes the datapack "world/meta" files so CommonDatapack::parseDatapack loads
// a complete, startable datapack: the player skin, player/start.xml (a starter
// profile), reputation.xml (the synthetic "nation" one + one per Tuxemon
// faction), event.xml, map/layers.xml, map/music.xml + every db/music track
// transcoded to opus, the per-terrain battle backgrounds from db/environment,
// the region informations.xml + start.xml, and empty plants/crafting files.
// Every writer returns false on I/O failure (ENOSPC/permission) so main can
// exit non-zero instead of reporting success over a corrupt datapack.

#include "TuxemonDb.hpp"
#include "Localization.hpp"
#include "DatapackWriter.hpp"
#include "SkinGen.hpp"

#include <string>

namespace tuxemon {

class WorldWriter {
public:
    WorldWriter(const TuxemonDb &db, const Localization &l10n,
                const std::string &modRoot, const std::string &outRoot,
                SkinGen &skins, const DatapackWriter &dw,
                const std::string &startMap, int startX, int startY);

    bool writeAll();

    // SHARED CONVENTION with the map converter (per-map backgroundsound refs):
    // a music track slug maps to the datapack file "music/"+sanitizeTrackSlug(slug)
    // +".opus".  sanitize = tolower, then every char not in [a-z0-9] -> '-'.
    static std::string sanitizeTrackSlug(const std::string &slug);

private:
    int chooseStarterMonster() const;  // a basic-stage monster txmn id
    int chooseCaptureItem() const;     // an item id for captured_with

    bool writePlayerSkin();
    bool writePlayerStart(int starterId, int captureItemId);
    bool writeReputation();
    bool writeEvent();
    bool writeLayers(int captureItemId);
    bool writeVisualCategory();
    bool writeFightBackgrounds();
    bool writeMusic();
    bool writeRegionInfo();
    bool writeRegionStart();
    bool writeEmptyPlantsAndCrafting();

    const TuxemonDb &db_;
    const Localization &l10n_;
    std::string modRoot_;
    std::string outRoot_;
    SkinGen &skins_;
    const DatapackWriter &dw_;
    std::string startMap_;
    int startX_;
    int startY_;
};

} // namespace tuxemon

#endif // TUXEMON2CC_WORLDWRITER_HPP
