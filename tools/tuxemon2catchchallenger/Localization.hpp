#ifndef TUXEMON2CC_LOCALIZATION_HPP
#define TUXEMON2CC_LOCALIZATION_HPP

// Parses Tuxemon gettext .po catalogues (l18n/<locale>/LC_MESSAGES/base.po) into
// a slug -> text map.  Tuxemon keys display names by the entity slug and
// descriptions by "<slug>_description".  We load English (the CC default,
// emitted with no lang attribute) and French (lang="fr").

#include <string>
#include <unordered_map>

namespace tuxemon {

class Localization {
public:
    Localization();
    // modRoot = .../mods/tuxemon
    void load(const std::string &modRoot);

    // Display name; falls back to a title-cased slug when not translated.
    std::string nameEn(const std::string &slug) const;
    std::string nameFr(const std::string &slug) const; // "" => no French entry
    std::string descEn(const std::string &slug) const; // "" => none
    std::string descFr(const std::string &slug) const; // "" => none

private:
    static void parsePo(const std::string &file, std::unordered_map<std::string,std::string> &out);
    std::string lookup(const std::unordered_map<std::string,std::string> &m,
                       const std::string &key) const;

    std::unordered_map<std::string,std::string> en_;
    std::unordered_map<std::string,std::string> fr_;
};

} // namespace tuxemon

#endif // TUXEMON2CC_LOCALIZATION_HPP
