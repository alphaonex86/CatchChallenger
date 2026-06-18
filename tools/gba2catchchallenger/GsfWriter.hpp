#ifndef GBA2CC_GSFWRITER_HPP
#define GBA2CC_GSFWRITER_HPP

// Emit a standard GSF set (one <label>.gsflib + per-song <name>.minigsf) from a
// Gen3 GBA ROM, so the music plays both in external GSF players (foobar2000
// foo_input_gsf, Highly Advanced) AND in the CatchChallenger client (which
// decodes the gsflib's ROM image with the software MP2k synth — no emulation).
//
// The gsflib is the ROM with the loveemu/saptapper MP2k GSF driver installed in
// free space (so an emulator boots straight into "init sound, play song N").  We
// locate the engine functions relative to m4aSongNumStart (the routine M4aRipper
// already finds) and patch the driver's call slots.  We also tag the gsflib with
// a custom _cc_songtable so OUR client never has to re-scan.

#include "Gba.hpp"
#include "M4aRipper.hpp"

#include <cstdint>
#include <string>
#include <vector>

class GsfWriter {
public:
    GsfWriter();

    // Locate the engine functions (anchored on M4aRipper's already-found
    // SelectSong/gSongTable) and build the driver-installed ROM image.  Returns
    // false if the ROM has no free space for the driver or SelectSong wasn't
    // located.  init/main/vsync are best-effort: if one is missing the gsflib is
    // still built (our client ignores the driver) but a warning is logged because
    // external emulator playback may then fail.
    bool prepare(const GbaRom &rom, const M4aRipper &ripper);

    // Write the gsflib (the driver-installed ROM image) with the _cc_songtable /
    // _cc_songcount tags.  `gameName` goes into the title/game tags.
    bool writeLib(const std::string &libPath, const std::string &gameName) const;

    // Write one minigsf selecting song `songId`, referencing the gsflib by its
    // dir-relative filename `libRelName` (the _lib tag).  `title` is optional.
    bool writeMinigsf(const std::string &path, int songId,
                      const std::string &libRelName, const std::string &title) const;

private:
    std::vector<uint8_t> rom_;     // working copy with the driver installed
    uint32_t driverOffset_;        // file offset where the driver was placed
    uint32_t songTableAddr_;       // GBA address of gSongTable
    int songCount_;
    int songIdWidth_;              // minigsf payload width (1..4 bytes)
    bool initOk_, mainOk_, vsyncOk_;
};

#endif // GBA2CC_GSFWRITER_HPP
