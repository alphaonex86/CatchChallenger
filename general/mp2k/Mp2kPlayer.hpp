#ifndef CATCHCHALLENGER_MP2KPLAYER_HPP
#define CATCHCHALLENGER_MP2KPLAYER_HPP

// Portable MP2k / "Sappy" software player (the GBA Gen3 music engine).
//
// The synth here is the SAME interpreter the gba2catchchallenger ripper uses to
// render a ROM to opus; it is abstracted over a tiny byte SOURCE so the exact
// same code renders either directly from a GBA ROM (converter) or from the ROM
// image carried inside a GSF/MiniGSF (game client — it decodes WITHOUT emulating
// the GBA CPU; it just reads the song data the gsflib contains and synthesises).
//
// A song in the ROM is a graph of absolute GBA pointers (song header -> track
// byte-streams -> voicegroup -> 8-bit PCM samples); file offset == GBA pointer
// minus 0x08000000.  No Qt, no STL beyond <vector>; safe on the constrained
// targets the engine runs on.

#include <cstdint>
#include <cstddef>
#include <vector>

namespace CatchChallenger {

// Read-only byte view the player needs.  Offsets are FILE offsets (a GBA pointer
// 0x08xxxxxx maps to offset = value-0x08000000), exactly like the ROM accessor.
class Mp2kSource
{
public:
    virtual ~Mp2kSource() {}
    virtual uint8_t u8(uint32_t offset) const = 0;
    virtual uint16_t u16(uint32_t offset) const = 0;
    virtual uint32_t u32(uint32_t offset) const = 0;
    // Read the 32-bit GBA pointer at offset; *ok=false for NULL/out-of-range.
    virtual uint32_t pointer(uint32_t offset, bool *ok) const = 0;
    virtual bool isPointer(uint32_t value) const = 0;
    virtual uint32_t size() const = 0; // ROM size (bounds parity for blob source)
};

// Render one song to an interleaved stereo float buffer (L,R,L,R,...) at
// sampleRate Hz for up to `seconds`.  songHeaderOffset is the file offset of the
// song header (u8 trackCount, u32 voicegroupPtr@+4, u32 trackPtr[]@+8).
void mp2kRenderSong(const Mp2kSource &src, uint32_t songHeaderOffset,
                    int sampleRate, double seconds, std::vector<float> &outStereo);

// Flat ROM-image source: a contiguous GBA ROM image loaded at 0x08000000, so a
// file offset == (GBA pointer - 0x08000000) directly.  Used to render from a GSF
// gsflib's decompressed ROM image (client) and to wrap the ROM in the converter.
// The backing buffer must outlive the source (not copied).
class Mp2kRomSource : public Mp2kSource
{
public:
    Mp2kRomSource(const uint8_t *image, uint32_t imageSize);
    uint8_t u8(uint32_t offset) const override;
    uint16_t u16(uint32_t offset) const override;
    uint32_t u32(uint32_t offset) const override;
    uint32_t pointer(uint32_t offset, bool *ok) const override;
    bool isPointer(uint32_t value) const override;
    uint32_t size() const override { return imageSize_; }
private:
    const uint8_t *image_;
    uint32_t imageSize_;
};

} // namespace CatchChallenger

#endif
