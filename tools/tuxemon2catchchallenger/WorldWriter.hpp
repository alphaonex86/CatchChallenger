#ifndef TUXEMON2CC_WORLDWRITER_HPP
#define TUXEMON2CC_WORLDWRITER_HPP

// Writes the datapack "world/meta" files so CommonDatapack::parseDatapack loads
// a complete, startable datapack: the player skin, player/start.xml (a starter
// profile), reputation.xml, event.xml, map/layers.xml, map/music.xml (with a
// few transcoded tracks), the region informations.xml + start.xml, and empty
// plants/crafting files.

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

    void writeAll();

private:
    int chooseStarterMonster() const;  // a basic-stage monster txmn id
    int chooseCaptureItem() const;     // an item id for captured_with

    void writePlayerSkin();
    void writePlayerStart(int starterId, int captureItemId);
    void writeReputation();
    void writeEvent();
    void writeLayers(int captureItemId);
    void writeVisualCategory();
    void writeFightBackgrounds();
    void writeMusic();
    void writeRegionInfo();
    void writeRegionStart();
    void writeEmptyPlantsAndCrafting();

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
