#ifndef CATCHCHALLENGER_MP2KPLAYER_HPP
#define CATCHCHALLENGER_MP2KPLAYER_HPP

// Portable MP2k / "Sappy" software player (the GBA Gen3 music engine).
//
// The synth here is the SAME interpreter the gba2catchchallenger ripper uses to
// render the ROM to opus; it is abstracted over a tiny byte SOURCE so the exact
// same code renders either directly from a GBA ROM (converter) or from a small
// self-contained BLOB extracted from the ROM (game client — it has no ROM).
//
// A song in the ROM is a graph of absolute GBA pointers (song header -> track
// byte-streams -> voicegroup -> 8-bit PCM samples).  The blob is just the set of
// ROM byte-ranges the render actually touches, keyed by their original file
// offset, so the player resolves the same pointers with no relocation.  No Qt,
// no STL beyond <vector>; safe on the constrained targets the engine runs on.

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

// ── Self-contained blob (extracted by the converter, played by the client) ──
// Layout: "MP2K"(4) u8 version u8 pad[3] u32 songHeaderOffset u32 romSize
//         u32 nSeg  then nSeg x { u32 off, u32 len, u8 bytes[len] } (off ascending).

// Records every byte a render touches over a base source (converter only); after
// rendering, serialize() emits the blob covering exactly those bytes.
class Mp2kRecordingSource : public Mp2kSource
{
public:
    Mp2kRecordingSource(const Mp2kSource &base);
    uint8_t u8(uint32_t offset) const override;
    uint16_t u16(uint32_t offset) const override;
    uint32_t u32(uint32_t offset) const override;
    uint32_t pointer(uint32_t offset, bool *ok) const override;
    bool isPointer(uint32_t value) const override;
    uint32_t size() const override;
    // Coalesce touched bytes into segments and write the blob.
    void serialize(uint32_t songHeaderOffset, std::vector<uint8_t> &out) const;
private:
    void mark(uint32_t offset, uint32_t len) const;
    const Mp2kSource &base_;
    mutable std::vector<bool> touched_; // one bit per ROM byte
};

// Blob-backed source: resolves reads via the captured segments.
class Mp2kBlobSource : public Mp2kSource
{
public:
    Mp2kBlobSource(const uint8_t *blob, size_t len);
    bool valid() const { return valid_; }
    uint32_t songHeaderOffset() const { return songHeaderOffset_; }
    uint8_t u8(uint32_t offset) const override;
    uint16_t u16(uint32_t offset) const override;
    uint32_t u32(uint32_t offset) const override;
    uint32_t pointer(uint32_t offset, bool *ok) const override;
    bool isPointer(uint32_t value) const override;
    uint32_t size() const override { return romSize_; }
private:
    struct Seg { uint32_t off; uint32_t len; const uint8_t *data; };
    const Seg *find(uint32_t offset) const;
    std::vector<Seg> segs_; // sorted by off
    uint32_t songHeaderOffset_;
    uint32_t romSize_;
    bool valid_;
};

// Convenience: parse a blob and render it (client entry point).  Returns false on
// a malformed blob or empty render.
bool mp2kRenderBlob(const uint8_t *blob, size_t len,
                    int sampleRate, double seconds, std::vector<float> &outStereo);

} // namespace CatchChallenger

#endif
