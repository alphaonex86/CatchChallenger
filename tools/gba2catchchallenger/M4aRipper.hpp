#ifndef GBA2CC_M4ARIPPER_HPP
#define GBA2CC_M4ARIPPER_HPP

// Renders a Gen3 M4A/"Sappy" song from the ROM to PCM and encodes it to opus.
// Locates gSongTable via the SelectSong Thumb signature, interprets each track's
// command stream, decodes the voicegroup (DirectSound samples + CGB channels),
// and mixes a stereo buffer (13379 Hz, the engine's default), then shells out to
// ffmpeg for opus.  A self-contained software MP2k player — no external ripper.

#include "Gba.hpp"

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

private:
    uint32_t songTable_;
    int songCount_;
};

#endif // GBA2CC_M4ARIPPER_HPP
