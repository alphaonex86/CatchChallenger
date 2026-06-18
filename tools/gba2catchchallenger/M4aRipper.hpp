#ifndef GBA2CC_M4ARIPPER_HPP
#define GBA2CC_M4ARIPPER_HPP

// Locates gSongTable (SelectSong Thumb signature) and renders/extracts Gen3 M4A/
// "Sappy" songs.  The actual interpreter+mixer lives in general/mp2k (shared with
// the game client); here we wrap the ROM as an Mp2kSource and either:
//   * render -> WAV -> opus (default datapack output), or
//   * extract a self-contained .mp2k BLOB (--original-sounds) that the client's
//     own copy of the same player decodes at runtime (no ROM needed there).

#include "Gba.hpp"
#include "../../general/mp2k/Mp2kPlayer.hpp"

#include <cstdint>
#include <string>
#include <vector>

class M4aRipper {
public:
    M4aRipper();
    // Locate gSongTable; returns false if not found.
    bool locate(const GbaRom &rom);
    int songCount() const { return songCount_; }

    // Render song to a stereo float buffer (interleaved L,R) at sampleRate Hz.
    // seconds caps the duration (BGM loops forever).  Empty if the song is silent.
    std::vector<float> render(const GbaRom &rom, int songId, int sampleRate, double seconds) const;

    // render() + write <outPath>.opus via ffmpeg.  Returns false on any failure.
    bool writeOpus(const GbaRom &rom, int songId, const std::string &outPath,
                   double seconds = 32.0) const;

    // Extract a self-contained .mp2k blob (the song subgraph the render touches)
    // and write it to outPath.  Verifies the blob round-trips (blob render ==
    // ROM render) before returning true.
    bool writeMp2kBlob(const GbaRom &rom, int songId, const std::string &outPath,
                       double seconds = 16.0) const;

private:
    // File offset of song header, or 0 (with *ok=false) if invalid.
    uint32_t songHeader(const GbaRom &rom, int songId, bool *ok) const;
    uint32_t songTable_;
    int songCount_;
};

#endif // GBA2CC_M4ARIPPER_HPP
