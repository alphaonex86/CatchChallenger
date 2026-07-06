#ifndef TUXEMON2CC_DATAPACKWRITER_HPP
#define TUXEMON2CC_DATAPACKWRITER_HPP

// Emits a CatchChallenger datapack from the loaded Tuxemon data:
//   informations.xml
//   monsters/type.xml            (Tuxemon's 13 elements + matchup chart)
//   monsters/buff/buff.xml       (Tuxemon statuses)
//   monsters/skill/skill.xml     (Tuxemon techniques, + the mandatory id=0)
//   monsters/monster.xml         (stats from shape, moveset, evolutions)
//   monsters/<id>/{front,back,small}.png
//   items/items.xml + items/tuxemon/<slug>.png
//
// All XML is written by hand (std::ofstream) so the formatting matches the
// official datapack.  Slugs are resolved to the numeric ids the engine wants
// through stable per-entity maps built once up-front.

#include "TuxemonDb.hpp"
#include "Localization.hpp"

#include <string>
#include <unordered_map>
#include <fstream>

namespace tuxemon {

class DatapackWriter {
public:
    DatapackWriter(const TuxemonDb &db, const Localization &l10n,
                   const std::string &modRoot, const std::string &outRoot);

    bool writeAll();

    // Public id lookups (valid after writeAll); -1 when unknown.
    int idForItem(const std::string &slug) const;
    int idForMonster(const std::string &slug) const;

private:
    void buildIdMaps();

    // each returns false on any I/O failure (mkpath, open, stream state)
    bool writeInformations();
    bool writeTypes();
    bool writeBuffs();
    bool writeSkills();
    bool writeMonsters();
    bool writeItems();
    void writeSprites();

    // Emit <name>/<description> (en default + every loaded locale) at the
    // given indent for a slug.
    void writeNameDesc(std::ofstream &o, const char *indent,
                       const std::string &slug, bool withDescription);

    // id lookups (return -1 when the slug is unknown)
    int skillId(const std::string &slug) const;
    int buffId(const std::string &slug) const;
    int monsterId(const std::string &slug) const;

    const TuxemonDb &db_;
    const Localization &l10n_;
    std::string modRoot_;
    std::string outRoot_;

    std::unordered_map<std::string,int> techToId_;    // technique slug -> skill id
    std::unordered_map<std::string,int> statusToId_;  // status slug   -> buff id
    std::unordered_map<std::string,int> itemToId_;    // item slug     -> item id
    std::unordered_map<std::string,int> monsterToId_; // monster slug  -> monster id
};

} // namespace tuxemon

#endif // TUXEMON2CC_DATAPACKWRITER_HPP
