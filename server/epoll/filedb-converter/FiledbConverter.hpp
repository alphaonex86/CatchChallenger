#ifndef FILEDB_CONVERTER_HPP
#define FILEDB_CONVERTER_HPP

#include <string>

namespace FiledbConverter {

// File kind, inferred from the on-disk path under database/.
enum class FileKind {
    Unknown,
    ServerCounters,       // database/server/server
    DictionaryMap,        // database/server/dictionary_map
    DictionaryReputation, // database/common/dictionary_reputation
    DictionarySkin,       // database/common/dictionary_skin
    DictionaryStarter,    // database/common/dictionary_starter
    Login,                // database/login/{HASH_HEX}
    Account,              // database/common/accounts/{ACCOUNT_ID}
    CharacterCommon,      // database/common/characters/{PSEUDO_HEX}
    CharacterServer,      // database/server/characters/{PSEUDO_HEX}
    Clan,                 // database/server/clans/{CLAN_ID}
    Zone                  // database/server/zone/{ZONE_NAME}
};

FileKind detectKind(const std::string &path);
const char *kindName(FileKind k);

// Direction of conversion.
enum class Direction { BinToXml, XmlToBin };

// Single-file conversion. Returns 0 on success.
int convertFile(const std::string &path, Direction dir);

// Recursive directory conversion. Returns 0 on success (even if some files
// were skipped as Unknown; those are logged but not fatal).
int convertDirectory(const std::string &dir, Direction d);

} // namespace FiledbConverter

#endif
