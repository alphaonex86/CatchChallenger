#ifndef GBA2CC_GEN3TEXT_HPP
#define GBA2CC_GEN3TEXT_HPP

// Decodes Gen3 (Latin) strings — used only for functional identifiers: map
// section names, species names and item names.  Not used to transcribe NPC
// dialogue.

#include "Gba.hpp"

#include <cstdint>
#include <string>
#include <vector>

class Gen3Text {
public:
    // Decode a 0xFF-terminated Gen3 string at a file offset to UTF-8-ish ASCII.
    static std::string decode(const GbaRom &rom, uint32_t offset, size_t maxLen=64);
    // Decode a sign string into PAGES: split on the paragraph/scroll control
    // codes (each becomes a separate text step), newline -> "<br />".  Buffered
    // variables and formatting controls are skipped.
    static std::vector<std::string> decodeSign(const GbaRom &rom, uint32_t offset, size_t maxLen=512);
    // Filesystem/datapack-safe slug: lowercase, spaces/punctuation -> '-'.
    static std::string slug(const std::string &s);
    // Title-case-ish display form (kept mostly as decoded, trimmed).
    static std::string display(const std::string &s);
};

#endif // GBA2CC_GEN3TEXT_HPP
