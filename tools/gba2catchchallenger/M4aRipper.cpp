#include "M4aRipper.hpp"

#include <QProcess>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QFileInfo>

#include <cmath>
#include <cstring>
#include <cstdio>
#include <iostream>

M4aRipper::M4aRipper() : songTable_(0), songCount_(0) {}

// little-endian byte writers (project style forbids lambdas)
static void putBytes(QByteArray &b, const char *s, int n) { b.append(s, n); }
static void putU16(QByteArray &b, uint16_t v) { b.append((char)(v & 0xFF)); b.append((char)((v >> 8) & 0xFF)); }
static void putU32(QByteArray &b, uint32_t v) { b.append((char)(v & 0xFF)); b.append((char)((v >> 8) & 0xFF)); b.append((char)((v >> 16) & 0xFF)); b.append((char)((v >> 24) & 0xFF)); }

// ── note-length / wait table (49 entries) ──
static const uint8_t kClock[49] = {
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
    28,30,32,36,40,42,44,48,52,54,56,60,64,66,68,72,76,78,80,84,88,90,92,96 };

// ── locate gSongTable via the SelectSong Thumb signature ──
bool M4aRipper::locate(const GbaRom &rom)
{
    static const uint8_t SIG_A[0x1E] = {
        0x00,0xB5,0x00,0x04,0x07,0x4A,0x08,0x49,0x40,0x0B,0x40,0x18,0x83,0x88,0x59,0x00,
        0xC9,0x18,0x89,0x00,0x89,0x18,0x0A,0x68,0x01,0x68,0x10,0x1C,0x00,0xF0 };
    static const uint8_t SIG_B[0x1E] = {
        0x00,0xB5,0x00,0x04,0x07,0x4B,0x08,0x49,0x40,0x0B,0x40,0x18,0x82,0x88,0x51,0x00,
        0x89,0x18,0x89,0x00,0xC9,0x18,0x0A,0x68,0x01,0x68,0x10,0x1C,0x00,0xF0 };
    const uint8_t *base = rom.raw(0, rom.size());
    if(base == NULL)
        return false;
    uint32_t off = 0;
    uint32_t found = 0;
    while(off + 0x1E <= rom.size())
    {
        if(std::memcmp(base + off, SIG_A, 0x1E) == 0 || std::memcmp(base + off, SIG_B, 0x1E) == 0)
        { found = off; break; }
        off += 2;
    }
    if(found == 0)
        return false;
    bool ok = false;
    songTable_ = rom.pointer(found + 40, &ok);
    if(!ok)
        return false;
    // count songs (stop on a long null run or an implausible header)
    int count = 0, nulls = 0;
    uint32_t p = songTable_;
    while(p + 8 <= rom.size() && count < 1500)
    {
        const uint32_t hdr = rom.u32(p);
        if(hdr == 0) { if(++nulls > 48) break; ++count; p += 8; continue; }
        nulls = 0;
        if(!rom.isPointer(hdr)) break;
        bool ok2 = false;
        const uint32_t ho = rom.pointer(p, &ok2);
        if(!ok2 || ho + 8 > rom.size() || rom.u8(ho) > 16) break;
        ++count; p += 8;
    }
    songCount_ = count;
    std::cerr << "M4aRipper: gSongTable=" << std::hex << songTable_ << std::dec
              << " songs=" << songCount_ << std::endl;
    return songCount_ > 0;
}

// ── ROM as an Mp2kSource: the shared player (general/mp2k) reads through this ──
namespace {
class RomMp2kSource : public CatchChallenger::Mp2kSource {
public:
    explicit RomMp2kSource(const GbaRom &rom) : rom_(rom) {}
    uint8_t u8(uint32_t o) const override { return rom_.u8(o); }
    uint16_t u16(uint32_t o) const override { return rom_.u16(o); }
    uint32_t u32(uint32_t o) const override { return rom_.u32(o); }
    uint32_t pointer(uint32_t o, bool *ok) const override { return rom_.pointer(o, ok); }
    bool isPointer(uint32_t v) const override { return rom_.isPointer(v); }
    uint32_t size() const override { return rom_.size(); }
private:
    const GbaRom &rom_;
};
}

uint32_t M4aRipper::songHeader(const GbaRom &rom, int songId, bool *ok) const
{
    if(ok) *ok = false;
    if(songTable_ == 0 || songId < 0 || songId >= songCount_)
        return 0;
    const uint32_t hdrPtr = rom.u32(songTable_ + (uint32_t)songId * 8);
    if(hdrPtr == 0 || !rom.isPointer(hdrPtr))
        return 0;
    return rom.pointer(songTable_ + (uint32_t)songId * 8, ok);
}

std::vector<float> M4aRipper::render(const GbaRom &rom, int songId, int R, double seconds) const
{
    std::vector<float> out;
    bool ok = false;
    const uint32_t h = songHeader(rom, songId, &ok);
    if(!ok) return out;
    RomMp2kSource src(rom);
    CatchChallenger::mp2kRenderSong(src, h, R, seconds, out);
    return out;
}

bool M4aRipper::writeOpus(const GbaRom &rom, int songId, const std::string &outPath, double seconds) const
{
    const int R = 13379;
    std::vector<float> pcm = render(rom, songId, R, seconds);
    if(pcm.size() < (std::size_t)R) // < ~0.5s of stereo -> silent/empty
        return false;
    // peak-normalise, then short fade-out
    float peak = 0.0f; std::size_t i = 0; while(i < pcm.size()) { const float a = std::fabs(pcm[i]); if(a > peak) peak = a; ++i; }
    const float norm = peak > 0.01f ? 0.85f / peak : 1.0f;
    const std::size_t frames = pcm.size() / 2;
    const std::size_t fade = R; // 1s fade
    i = 0;
    while(i < frames)
    {
        float g = norm;
        if(i + fade > frames) g *= (float)(frames - i) / fade;
        pcm[i * 2] *= g; pcm[i * 2 + 1] *= g;
        ++i;
    }
    // write a 16-bit stereo WAV to a temp file
    const QString wav = QString::fromStdString(outPath) + ".wav";
    QFile f(wav);
    if(!f.open(QIODevice::WriteOnly))
        return false;
    const uint32_t dataBytes = (uint32_t)(pcm.size() * 2);
    const uint32_t rate = (uint32_t)R;
    QByteArray hdr;
    putBytes(hdr, "RIFF", 4); putU32(hdr, 36 + dataBytes); putBytes(hdr, "WAVE", 4);
    putBytes(hdr, "fmt ", 4); putU32(hdr, 16); putU16(hdr, 1); putU16(hdr, 2);
    putU32(hdr, rate); putU32(hdr, rate * 4); putU16(hdr, 4); putU16(hdr, 16);
    putBytes(hdr, "data", 4); putU32(hdr, dataBytes);
    f.write(hdr);
    QByteArray samples;
    samples.reserve((int)dataBytes);
    i = 0;
    while(i < pcm.size()) { float v = pcm[i]; if(v > 1) v = 1; if(v < -1) v = -1; putU16(samples, (uint16_t)(int16_t)(v * 32767)); ++i; }
    f.write(samples);
    f.close();

    // Encode mono @48 kbps: the GBA source is signed 8-bit PCM (low-fidelity to
    // begin with), so a stereo/high-bitrate opus only stores upmix noise.  -ac 1
    // downmixes L/R to mono and 48k halves the file vs the old 96k stereo (~195 ->
    // ~95 KB/song).  Opus has no bit-depth setting; the 8-bit character comes from
    // the source, not an encoder flag.
    QStringList args;
    args << "-y" << "-i" << wav << "-c:a" << "libopus" << "-ac" << "1" << "-b:a" << "48k" << QString::fromStdString(outPath);
    QProcess p; p.start("ffmpeg", args);
    const bool fin = p.waitForFinished(60000);
    QFile::remove(wav);
    return fin && p.exitCode() == 0 && QFile::exists(QString::fromStdString(outPath));
}

bool M4aRipper::writeMp2kBlob(const GbaRom &rom, int songId, const std::string &outPath, double seconds) const
{
    bool ok = false;
    const uint32_t h = songHeader(rom, songId, &ok);
    if(!ok) return false;
    // Record exactly the bytes the render touches over the play window, then
    // serialize them into a self-contained blob (no relocation: original file
    // offsets are kept, so the same player resolves the same pointers).
    RomMp2kSource rsrc(rom);
    CatchChallenger::Mp2kRecordingSource rec(rsrc);
    std::vector<float> romPcm;
    CatchChallenger::mp2kRenderSong(rec, h, 13379, seconds, romPcm);
    if(romPcm.size() < 13379) // < ~0.5s stereo -> silent/empty, skip
        return false;
    std::vector<uint8_t> blob;
    rec.serialize(h, blob);

    // Verify the blob round-trips: decoding it must reproduce the ROM render
    // sample-for-sample (same player, same bytes) — a missed segment would diverge.
    std::vector<float> blobPcm;
    if(!CatchChallenger::mp2kRenderBlob(blob.data(), blob.size(), 13379, seconds, blobPcm))
        return false;
    if(blobPcm.size() != romPcm.size())
        return false;
    std::size_t i = 0;
    while(i < romPcm.size()) { if(romPcm[i] != blobPcm[i]) return false; ++i; }

    QFile f(QString::fromStdString(outPath));
    if(!f.open(QIODevice::WriteOnly))
        return false;
    f.write(reinterpret_cast<const char *>(blob.data()), (qint64)blob.size());
    f.close();
    return true;
}
