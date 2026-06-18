#include "GsfWriter.hpp"
#include "../../general/mp2k/Gsf.hpp"

#include <cstring>
#include <iostream>
#include <sstream>

// ── loveemu/saptapper MP2k GSF driver (244 bytes) ──
// Installed into ROM free space; ROM offset 0 gets an ARM `B` into it.  The five
// patch words are filled by InstallDriver: 0xD8 m4aSoundInit|1, 0xDC
// m4aSongNumStart|1, 0xE0 m4aSoundMain|1, 0xE4 m4aSoundVSync|1, 0xE8 song number
// (the minigsf overlays 0xE8).  In the shipped blob 0xD8..0xE4 are 0 and 0xE8 is
// 1; the three preceding literals are 0x03007FFC / 0x04000000 / 0x04000200.
static const unsigned char kGsfDriver[244] = {
    // 0x00 ARM entry + Thumb switch + watermark-skip loop
    0x01,0x10,0x8F,0xE2, 0x11,0xFF,0x2F,0xE1, 0x02,0xA0,0x01,0x68, 0x04,0x30,0x0A,0x0E,
    0xFB,0xD1,0x1F,0xE0,
    // 0x14 watermark "Sappy Driver Ripper by CaitSith2\Zoopd, (c) 2004, 2014 loveemu\0\0"
    0x53,0x61,0x70,0x70,0x79,0x20,0x44,0x72,0x69,0x76,0x65,0x72,0x20,0x52,0x69,0x70,
    0x70,0x65,0x72,0x20,0x62,0x79,0x20,0x43,0x61,0x69,0x74,0x53,0x69,0x74,0x68,0x32,
    0x5C,0x5A,0x6F,0x6F,0x70,0x64,0x2C,0x20,0x28,0x63,0x29,0x20,0x32,0x30,0x30,0x34,
    0x2C,0x20,0x32,0x30,0x31,0x34,0x20,0x6C,0x6F,0x76,0x65,0x65,0x6D,0x75,0x00,0x00,
    // 0x54 Thumb init + main loop
    0x00,0xB5,0x20,0x4B,0x00,0xF0,0x48,0xF8,0x1B,0x48,0x09,0xA1,0x01,0x60,0x1B,0x48,
    0x08,0x21,0x41,0x60,0x01,0x21,0x1A,0x48,0x01,0x60,0x81,0x60,0x1D,0x48,0x1A,0x4B,
    0x00,0xF0,0x3A,0xF8,0x19,0x4B,0x00,0xF0,0x37,0xF8,0x05,0xDF,0xFA,0xE7,0x00,0x00,
    // 0x84 ARM irq_handler
    0x48,0x30,0x9F,0xE5,0x00,0x10,0x93,0xE5,0x21,0x08,0x01,0xE0,0x0F,0x40,0x2D,0xE9,
    0x25,0x10,0x8F,0xE2,0x01,0x20,0x10,0xE2,0x00,0x00,0x00,0x1A,0x01,0x00,0x00,0xEA,
    0x0F,0xE0,0xA0,0xE1,0x11,0xFF,0x2F,0xE1,0x0F,0x40,0xBD,0xE8,0xB2,0x00,0xC3,0xE1,
    0x10,0x30,0x9F,0xE5,0x04,0x00,0x03,0xE5,0x1E,0xFF,0x2F,0xE1,
    // 0xC0 Thumb vsync_callback
    0x00,0xB5,0x08,0x4B,0x00,0xF0,0x12,0xF8,0x01,0xBC,0x00,0x47,
    // 0xCC literal pool: 0x03007FFC, 0x04000000, 0x04000200, then the 5 patch slots
    0xFC,0x7F,0x00,0x03, 0x00,0x00,0x00,0x04, 0x00,0x02,0x00,0x04,
    0x00,0x00,0x00,0x00,  // 0xD8 init_fn
    0x00,0x00,0x00,0x00,  // 0xDC select_song_fn
    0x00,0x00,0x00,0x00,  // 0xE0 main_fn
    0x00,0x00,0x00,0x00,  // 0xE4 vsync_fn
    0x01,0x00,0x00,0x00,  // 0xE8 song number
    0x00,0x2B,0x00,0xD0,0x18,0x47,0x70,0x47   // bx_r3 helper
};
static const uint32_t kInitFnOff = 0xD8, kSelectFnOff = 0xDC, kMainFnOff = 0xE0,
                      kVSyncFnOff = 0xE4, kSongNumOff = 0xE8;

static uint32_t rd32Arr(const unsigned char *r, uint32_t o)
{ return (uint32_t)r[o] | ((uint32_t)r[o+1]<<8) | ((uint32_t)r[o+2]<<16) | ((uint32_t)r[o+3]<<24); }
static void wr32(std::vector<uint8_t> &r, uint32_t o, uint32_t v)
{ r[o]=v&0xFF; r[o+1]=(v>>8)&0xFF; r[o+2]=(v>>16)&0xFF; r[o+3]=(v>>24)&0xFF; }

// Self-check the embedded driver image once (catches a bad paste at startup).
static bool driverBlobSane()
{
    if(sizeof(kGsfDriver) != 244) return false;
    if(rd32Arr(kGsfDriver, kInitFnOff)!=0 || rd32Arr(kGsfDriver, kSelectFnOff)!=0 ||
       rd32Arr(kGsfDriver, kMainFnOff)!=0 || rd32Arr(kGsfDriver, kVSyncFnOff)!=0) return false;
    if(rd32Arr(kGsfDriver, kSongNumOff)!=1) return false;
    if(rd32Arr(kGsfDriver, 0xCC)!=0x03007FFC || rd32Arr(kGsfDriver, 0xD0)!=0x04000000 ||
       rd32Arr(kGsfDriver, 0xD4)!=0x04000200) return false;
    return true;
}

GsfWriter::GsfWriter()
    : driverOffset_(0), songTableAddr_(0), songCount_(0), songIdWidth_(1),
      initOk_(false), mainOk_(false), vsyncOk_(false) {}

// Find the nearest 2-aligned match of a byte pattern scanning BACKWARD from
// `from`, within `range` bytes.  Returns the offset or 0 if not found.
static uint32_t findBackward(const std::vector<uint8_t> &rom, uint32_t from,
                             uint32_t range, const uint8_t *pat, uint32_t patLen)
{
    uint32_t d = 2;
    while(d <= range && d <= from)
    {
        uint32_t o = from - d;
        if(o + patLen <= rom.size() && std::memcmp(&rom[o], pat, patLen) == 0)
            return o;
        d += 2;
    }
    return 0;
}

// m4aSoundVSync: masked pattern A6 48 00 68 A6 4A 03 68 (bytes 0 and 4 wildcard),
// 4-aligned, within +/-0x1800 of `init`, guarded by BX LR (70 47) at +0x0C.
static uint32_t findVSync(const std::vector<uint8_t> &rom, uint32_t init)
{
    const uint32_t span = 0x1800;
    uint32_t lo = init > span ? init - span : 0;
    uint32_t hi = init + span;
    if(hi + 0x0E > rom.size()) hi = (uint32_t)rom.size() - 0x0E;
    uint32_t o = lo & ~3u;
    while(o <= hi)
    {
        if(rom[o+1]==0x48 && rom[o+2]==0x00 && rom[o+3]==0x68 &&
           rom[o+5]==0x4A && rom[o+6]==0x03 && rom[o+7]==0x68 &&
           rom[o+0x0C]==0x70 && rom[o+0x0D]==0x47)
            return o;
        o += 4;
    }
    // alternate prologue (non-Pokemon builds), searched forward
    o = init & ~3u;
    while(o + 0x0A <= rom.size() && o <= init + span)
    {
        if(rom[o]==0x00 && rom[o+1]==0xB5 && rom[o+3]==0x48 && rom[o+4]==0x02 &&
           rom[o+5]==0x68 && rom[o+6]==0x10 && rom[o+7]==0x68 && rom[o+9]==0x49)
            return o;
        o += 2;
    }
    return 0;
}

// First 4-aligned run of >= need bytes of a single padding value (0xFF then 0x00).
static uint32_t findFreeSpace(const std::vector<uint8_t> &rom, uint32_t need)
{
    const uint8_t pads[2] = { 0xFF, 0x00 };
    int pi = 0;
    while(pi < 2)
    {
        const uint8_t pad = pads[pi];
        uint32_t i = 0x200; // skip the header region
        while(i + need <= rom.size())
        {
            uint32_t run = 0;
            while(i + run < rom.size() && rom[i+run] == pad) ++run;
            if(run >= need)
                return (i + 3) & ~3u; // 4-align inside the run
            i += (run + 1);
        }
        ++pi;
    }
    return 0;
}

bool GsfWriter::prepare(const GbaRom &rom, const M4aRipper &ripper)
{
    if(!driverBlobSane())
    {
        std::cerr << "GsfWriter: embedded driver blob failed self-check — refusing." << std::endl;
        return false;
    }
    const uint32_t selectFn = ripper.selectSongFnOffset();
    const uint32_t songTableFileOff = ripper.songTableOffset();
    songCount_ = ripper.songCount();
    if(selectFn == 0 || songCount_ <= 0)
    {
        std::cerr << "GsfWriter: SelectSong/gSongTable not located — cannot build GSF." << std::endl;
        return false;
    }
    songTableAddr_ = 0x08000000u + songTableFileOff;
    songIdWidth_ = songCount_ < 256 ? 1 : (songCount_ < 65536 ? 2 : 4);

    // Copy the ROM and resolve the engine-function chain (file offsets).
    const uint8_t *base = rom.raw(0, rom.size());
    if(base == NULL) return false;
    rom_.assign(base, base + rom.size());

    static const uint8_t mainSig[2] = { 0x00, 0xB5 };
    uint32_t mainFn = findBackward(rom_, selectFn, 0x20, mainSig, 2);
    static const uint8_t initSigA[4] = { 0x70, 0xB5, 0x14, 0x48 };
    static const uint8_t initSigB[4] = { 0xF0, 0xB5, 0x47, 0x46 };
    // legacy mp2ktool fallback: the bare push-prologue (2 bytes) — the nearest
    // m4aSoundInit before main.  Less precise but catches builds whose 3rd/4th
    // prologue bytes differ from saptapper's two 4-byte signatures (e.g. FireRed).
    static const uint8_t initSig2A[2] = { 0x70, 0xB5 };
    static const uint8_t initSig2B[2] = { 0xF0, 0xB5 };
    uint32_t initFn = 0;
    if(mainFn != 0)
    {
        initFn = findBackward(rom_, mainFn, 0x100, initSigA, 4);
        if(initFn == 0) initFn = findBackward(rom_, mainFn, 0x100, initSigB, 4);
        if(initFn == 0) initFn = findBackward(rom_, mainFn, 0x100, initSig2A, 2);
        if(initFn == 0) initFn = findBackward(rom_, mainFn, 0x100, initSig2B, 2);
    }
    uint32_t vsyncFn = initFn ? findVSync(rom_, initFn) : 0;
    mainOk_ = mainFn != 0; initOk_ = initFn != 0; vsyncOk_ = vsyncFn != 0;

    // Place + install the driver.
    driverOffset_ = findFreeSpace(rom_, sizeof(kGsfDriver) + 16);
    if(driverOffset_ == 0)
    {
        std::cerr << "GsfWriter: no ROM free space for the GSF driver." << std::endl;
        return false;
    }
    std::memcpy(&rom_[driverOffset_], kGsfDriver, sizeof(kGsfDriver));
    // Patch the function slots (Thumb addresses, |1).  A missing function leaves
    // the slot 0 -> external playback may crash, but our client ignores it.
    if(initFn)  wr32(rom_, driverOffset_ + kInitFnOff,   (0x08000000u + initFn)  | 1u);
    if(selectFn)wr32(rom_, driverOffset_ + kSelectFnOff, (0x08000000u + selectFn)| 1u);
    if(mainFn)  wr32(rom_, driverOffset_ + kMainFnOff,   (0x08000000u + mainFn)  | 1u);
    if(vsyncFn) wr32(rom_, driverOffset_ + kVSyncFnOff,  (0x08000000u + vsyncFn) | 1u);
    // ARM `B` at ROM offset 0 -> driver:  0xEA000000 | ((dest-(0+8))/4 & 0xFFFFFF)
    const uint32_t armB = 0xEA000000u | (((driverOffset_ - 8u) >> 2) & 0x00FFFFFFu);
    wr32(rom_, 0, armB);

    std::cerr << "GsfWriter: driver@0x" << std::hex << (0x08000000u + driverOffset_)
              << " songTable=0x" << songTableAddr_ << std::dec
              << " songs=" << songCount_ << " (init=" << initOk_ << " main=" << mainOk_
              << " vsync=" << vsyncOk_ << ")";
    if(!(initOk_ && mainOk_ && vsyncOk_))
        std::cerr << "  [WARN external playback best-effort: a function was not found;"
                     " our client still plays it]";
    std::cerr << std::endl;
    return true;
}

bool GsfWriter::writeLib(const std::string &libPath, const std::string &gameName) const
{
    if(rom_.empty()) return false;
    std::vector<uint8_t> program =
        CatchChallenger::gsfBuildProgram(0x08000000u, 0x08000000u, rom_.data(), (uint32_t)rom_.size());
    std::ostringstream tags;
    tags << "utf8=1\n"
         << "_cc_songtable=0x" << std::hex << songTableAddr_ << std::dec << "\n"
         << "_cc_songcount=" << songCount_ << "\n"
         << "game=" << gameName << "\n";
    return CatchChallenger::gsfWriteFile(libPath, program, tags.str());
}

bool GsfWriter::writeMinigsf(const std::string &path, int songId,
                             const std::string &libRelName, const std::string &title) const
{
    if(driverOffset_ == 0) return false;
    // Skip the SAME ids the opus path skips (M4aRipper::render bails when
    // songId>=songCount_): an out-of-range id would silently truncate into
    // songIdWidth_ bytes and select the wrong song / a garbage table slot.
    if(songId < 0 || songId >= songCount_) return false;
    uint8_t payload[4] = { 0, 0, 0, 0 };
    uint32_t v = (uint32_t)songId;
    int i = 0;
    while(i < songIdWidth_) { payload[i] = (uint8_t)(v & 0xFF); v >>= 8; ++i; }
    const uint32_t songNumAddr = 0x08000000u + driverOffset_ + kSongNumOff;
    std::vector<uint8_t> program =
        CatchChallenger::gsfBuildProgram(0x08000000u, songNumAddr, payload, (uint32_t)songIdWidth_);
    std::ostringstream tags;
    tags << "utf8=1\n_lib=" << libRelName << "\n";
    if(!title.empty())
        tags << "title=" << title << "\n";
    return CatchChallenger::gsfWriteFile(path, program, tags.str());
}
