#ifndef GBA2CC_SPRITERIPPER_HPP
#define GBA2CC_SPRITERIPPER_HPP

// Extracts the Gen3 battle SPRITES (front + palette) for a list of species and
// writes monsters/<id>/{front,back,small}.png.  The front pic is a 64x64 4bpp
// LZ77-compressed image; the palette is an LZ77-compressed 16-colour BGR555
// table.  Tables are auto-located (tag-increment + LZ77-pointer signature).

#include "Gba.hpp"

#include <cstdint>
#include <string>
#include <vector>

class SpriteRipper {
public:
    SpriteRipper();
    // Auto-locate the sprite/palette tables in the ROM.  False if not found.
    bool locate(const GbaRom &rom);
    // Write front/back/small.png for one species id into <outRoot>/monsters/<id>/.
    // Returns false when the sprite can't be decoded.
    bool writeSpecies(const GbaRom &rom, const std::string &outRoot, int speciesId) const;

private:
    uint32_t frontTable_;   // gMonFrontPicTable ({u32 data,u16 size,u16 tag})
    uint32_t backTable_;    // gMonBackPicTable (same layout); 0 => mirror front
    uint32_t paletteTable_; // gMonPaletteTable ({u32 data,u16 tag,u16})
};

#endif // GBA2CC_SPRITERIPPER_HPP
