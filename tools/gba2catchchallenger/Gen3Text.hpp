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
    // STRICT decode for scanning/validating a section-name TABLE entry: returns
    // the name only if EVERY byte up to the 0xFF terminator is a valid name
    // character (length 2..maxLen); "" otherwise.  Unlike decode() it does NOT
    // silently drop control bytes, so random data does not pass as a name.
    static std::string strictName(const GbaRom &rom, uint32_t offset, size_t maxLen=24);
    // Decode a sign string into PAGES: split on the paragraph/scroll control
    // codes (each becomes a separate text step), newline -> "<br />".  Buffered
    // variables and formatting controls are skipped.
    static std::vector<std::string> decodeSign(const GbaRom &rom, uint32_t offset, size_t maxLen=512);
    // Flat one-line decode for DESCRIPTION text (item/move/Pokedex flavour):
    // newline/scroll controls collapse to single spaces.
    static std::string decodeParagraph(const GbaRom &rom, uint32_t offset, size_t maxLen=400);
    // Species name from gSpeciesNames (stride 11), Title-cased for display.
    // "" when the ROM has no species-name table (hacks) or internalId is 0.
    static std::string speciesName(const GbaRom &rom, uint16_t internalId);
    // Filesystem/datapack-safe slug: lowercase, spaces/punctuation -> '-'.
    static std::string slug(const std::string &s);
    // Title-case-ish display form (kept mostly as decoded, trimmed).
    static std::string display(const std::string &s);
};

#endif // GBA2CC_GEN3TEXT_HPP
