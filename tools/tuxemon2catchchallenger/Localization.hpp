#ifndef TUXEMON2CC_LOCALIZATION_HPP
#define TUXEMON2CC_LOCALIZATION_HPP

// Parses Tuxemon gettext .po catalogues (l18n/<locale>/LC_MESSAGES/base.po) into
// a slug -> text map.  Tuxemon keys display names by the entity slug and
// descriptions by "<slug>_description".  English is the CC default (emitted
// with no lang attribute); every other locale present under l18n/ is loaded
// under its short code (de_DE -> "de").  On a short-code collision
// (es_ES/es_MX) the alphabetically first locale wins.

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace tuxemon {

class Localization {
public:
    Localization();
    // modRoot = .../mods/tuxemon
    void load(const std::string &modRoot);

    // Display name; falls back to a title-cased slug when not translated.
    std::string nameEn(const std::string &slug) const;
    std::string descEn(const std::string &slug) const; // "" => none

    // Non-English locales, sorted short codes ("cs","de","eo",...).
    const std::vector<std::string> &locales() const;
    // "" => no entry for that locale.
    std::string name(const std::string &locale, const std::string &slug) const;
    std::string desc(const std::string &locale, const std::string &slug) const;

    // legacy accessors (delegate to locale "fr")
    std::string nameFr(const std::string &slug) const; // "" => no French entry
    std::string descFr(const std::string &slug) const; // "" => none

private:
    static void parsePo(const std::string &file, std::unordered_map<std::string,std::string> &out);
    std::string lookup(const std::unordered_map<std::string,std::string> &m,
                       const std::string &key) const;

    std::unordered_map<std::string,std::string> en_;
    std::map<std::string,std::unordered_map<std::string,std::string> > byLocale_;
    std::vector<std::string> locales_;
};

} // namespace tuxemon

#endif // TUXEMON2CC_LOCALIZATION_HPP
