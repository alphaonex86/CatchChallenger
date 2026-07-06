#ifndef GBA2CC_OVERWORLDSPRITE_HPP
#define GBA2CC_OVERWORLDSPRITE_HPP

// Renders a Gen3 overworld object-event graphic (NPC sprite) to an RGBA image:
// the down-facing standing frame.  Offsets/struct layout per ROM come from
// GameInfo (gObjectEventGraphicsInfoPointers + gObjectEventSpritePalettes).

#include "Gba.hpp"

#include <QImage>
#include <cstdint>
#include <vector>

class OverworldSprite {
public:
    // Render graphicsId to a 48x96 CatchChallenger trainer sheet (3 cols x 4
    // rows of 16x24: row0=up, row1=right, row2=down, row3=left; col1=standing).
    // Null image when out of range/invalid.
    static QImage render(const GbaRom &rom, uint8_t graphicsId);

    // Render graphicsId's FIRST frame at its native size, no character filter
    // (for static object-event graphics like the 16x16 item ball).
    static QImage renderStatic(const GbaRom &rom, uint8_t graphicsId);

private:
    // Locate the palette data (file offset) whose tag matches; 0 when absent.
    static uint32_t findPalette(const GbaRom &rom, uint16_t tag);
    // Build the 16-entry ARGB palette for palTag (all black when absent).
    static void buildPalette(const GbaRom &rom, uint16_t palTag, uint32_t *palette);
    // Decode one 4bpp frame to RGBA; null image when the entry is invalid.
    static QImage decodeFrame(const GbaRom &rom, uint32_t imagesPtr, int frameIndex,
                              int width, int height, const uint32_t *palette);
    // frames[idx], else frames[fallback], else frames[0], else null.
    static QImage frameOr(const std::vector<QImage> &frames, int idx, int fallback);
    // Crop to opaque content and bottom-centre into a 16x24 cell.
    static QImage fitCell(const QImage &frame);
};

#endif // GBA2CC_OVERWORLDSPRITE_HPP
