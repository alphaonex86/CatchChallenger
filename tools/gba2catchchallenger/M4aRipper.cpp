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

// ── per-track + per-voice synth state ──
namespace {
struct Trk {
    uint32_t pc;            // program counter (file offset); 0 => stopped
    uint8_t running;        // last repeatable command (>=0xBD)
    uint8_t key, vel;       // running note defaults
    int wait;               // ticks until next event
    uint32_t ret; bool inCall;
    int8_t keyShift;
    uint8_t voice, vol, pan;
    int bend; uint8_t bendRange, tune;
    int reptCount; uint32_t reptDest;
    bool stopped;
};
struct Voice {
    bool active;
    int kind;               // 0 directsound, 1 square, 3 wave, 4 noise
    // directsound sample
    uint32_t dataOff, len, loopStart; bool loop;
    double freqHz, pos;
    // cgb
    double phase, duty; uint32_t lfsr;
    // envelope (0..255), stepped at 60Hz
    int envState;           // 0 atk 1 dec 2 sus 3 rel 4 dead
    double env; uint8_t ea, ed, es, er;
    bool directsound;
    // gating / mix
    int gate;               // remaining note ticks; <0 = tied (until EOT)
    int vel; int volL, volR;
};
}

static double pitchHz(double sampleRate1024, int playedNote, int toneKey, int keyShift,
                      int bend, int bendRange, int tune)
{
    const double midiKeyPitch = playedNote + (60 - toneKey) + keyShift;
    const double pitch768 = (double)(bend - 64) * bendRange * 12.0 + (double)(tune - 64) * 12.0;
    return (sampleRate1024 / 1024.0) * std::pow(2.0, (midiKeyPitch - 60.0) / 12.0 + pitch768 / 768.0);
}

std::vector<float> M4aRipper::render(const GbaRom &rom, int songId, int R, double seconds) const
{
    std::vector<float> out;
    if(songTable_ == 0 || songId < 0 || songId >= songCount_)
        return out;
    const uint32_t hdrPtr = rom.u32(songTable_ + (uint32_t)songId * 8);
    if(hdrPtr == 0 || !rom.isPointer(hdrPtr))
        return out;
    bool ok = false;
    const uint32_t h = rom.pointer(songTable_ + (uint32_t)songId * 8, &ok);
    if(!ok) return out;
    const int trackCount = rom.u8(h);
    if(trackCount < 1 || trackCount > 16) return out;
    const uint32_t voicegroup = rom.pointer(h + 4, &ok);
    if(!ok) return out;

    std::vector<Trk> tk((std::size_t)trackCount);
    int t = 0;
    while(t < trackCount)
    {
        Trk &x = tk[t];
        std::memset(&x, 0, sizeof(Trk));
        x.pc = rom.pointer(h + 8 + (uint32_t)t * 4, &ok);
        x.vel = 100; x.vol = 127; x.pan = 64; x.bend = 64; x.bendRange = 2; x.tune = 64;
        x.stopped = !ok;
        ++t;
    }

    std::vector<Voice> vs;
    int tempoI = 150, tempoC = 0;
    const long maxS = (long)(R * seconds);
    const int frameSamp = R / 60;
    out.assign((std::size_t)maxS * 2, 0.0f);
    long si = 0;

    // execute one track until it WAITs / stops (notes don't consume ticks)
    // returns immediately on FINE.
    while(si < maxS)
    {
        // ── frame logic (60 Hz): advance ticks per tempo ──
        tempoC += tempoI;
        while(tempoC >= 150)
        {
            tempoC -= 150;
            // gate countdown / note-off
            std::size_t vi = 0;
            while(vi < vs.size())
            {
                if(vs[vi].active && vs[vi].gate > 0)
                {
                    if(--vs[vi].gate == 0) vs[vi].envState = 3; // release
                }
                ++vi;
            }
            // advance tracks
            t = 0;
            while(t < trackCount)
            {
                Trk &x = tk[t];
                if(x.stopped) { ++t; continue; }
                if(x.wait > 0) { --x.wait; if(x.wait > 0) { ++t; continue; } }
                // exec commands until a wait/stop
                int guard = 0;
                while(guard++ < 5000 && !x.stopped && x.wait == 0)
                {
                    uint8_t cmd = rom.u8(x.pc);
                    bool runArg = false;
                    if(cmd < 0x80) { cmd = x.running; runArg = true; }
                    else { ++x.pc; if(cmd >= 0xBD) x.running = cmd; }

                    if(cmd >= 0x80 && cmd <= 0xB0)        // WAIT
                    { x.wait = kClock[cmd - 0x80]; break; }
                    else if(cmd >= 0xCF)                  // NOTE
                    {
                        int gate = kClock[cmd - 0xCF];    // 0 (TIE) = held
                        uint8_t b0 = rom.u8(x.pc);
                        if(b0 < 0x80) { x.key = b0; ++x.pc;
                            uint8_t b1 = rom.u8(x.pc);
                            if(b1 < 0x80) { x.vel = b1; ++x.pc;
                                uint8_t b2 = rom.u8(x.pc);
                                if(b2 < 0x80) { gate += b2; ++x.pc; }
                            }
                        }
                        // build a voice
                        uint8_t program = x.voice;
                        uint8_t playedKey = (uint8_t)(x.key + x.keyShift);
                        // resolve keysplit / drumkit
                        uint32_t ve = voicegroup + (uint32_t)program * 12;
                        uint8_t type = rom.u8(ve);
                        uint8_t toneKey = rom.u8(ve + 1);
                        uint32_t wavPtr = rom.pointer(ve + 4, &ok);
                        uint8_t ea = rom.u8(ve + 8), ed = rom.u8(ve + 9), es = rom.u8(ve + 10), er = rom.u8(ve + 11);
                        if(type == 0x40) {                // keysplit
                            uint32_t subVG = rom.pointer(ve + 4, &ok);
                            uint32_t tbl = rom.pointer(ve + 8, &ok);
                            uint8_t idx = rom.u8(tbl + playedKey);
                            uint32_t se = subVG + (uint32_t)idx * 12;
                            type = rom.u8(se); toneKey = rom.u8(se + 1);
                            wavPtr = rom.pointer(se + 4, &ok);
                            ea = rom.u8(se + 8); ed = rom.u8(se + 9); es = rom.u8(se + 10); er = rom.u8(se + 11);
                        } else if(type == 0x80) {         // drumkit
                            uint32_t subVG = rom.pointer(ve + 4, &ok);
                            uint32_t se = subVG + (uint32_t)playedKey * 12;
                            type = rom.u8(se); toneKey = rom.u8(se + 1);
                            wavPtr = rom.pointer(se + 4, &ok);
                            ea = rom.u8(se + 8); ed = rom.u8(se + 9); es = rom.u8(se + 10); er = rom.u8(se + 11);
                            playedKey = toneKey; // drum plays at its own key
                        }
                        const int cls = type & 0x07;
                        Voice v; std::memset(&v, 0, sizeof(Voice));
                        v.active = true; v.gate = (gate == 0) ? -1 : gate; v.vel = x.vel;
                        v.ea = ea; v.ed = ed; v.es = es; v.er = er; v.env = 0; v.envState = 0;
                        // stereo volume from track vol + pan
                        const int y = 2 * x.pan - 128;   // -128..126
                        const int vx = x.vol; // 0..127
                        v.volR = ((y + 128) * vx) >> 8;
                        v.volL = ((127 - y) * vx) >> 8;
                        if((type & 0x07) == 0) {
                            v.directsound = true; v.kind = 0;
                            // WaveData: freq@+4, loopStart@+8, size@+12, pcm@+16
                            if(ok && wavPtr + 16 <= rom.size()) {
                                const uint32_t freq = rom.u32(wavPtr + 4);
                                v.loopStart = rom.u32(wavPtr + 8);
                                v.len = rom.u32(wavPtr + 12);
                                v.dataOff = wavPtr + 16;
                                v.loop = (rom.u16(wavPtr + 2) != 0) || v.loopStart < v.len;
                                if(type & 0x08)   // FIX: fixed rate, not pitched by note (drums)
                                    v.freqHz = (double)freq / 1024.0;
                                else
                                    v.freqHz = pitchHz((double)freq, playedKey, toneKey, 0, x.bend, x.bendRange, x.tune);
                                if(v.len > 0 && v.len < 4000000 && v.dataOff + v.len <= rom.size())
                                    vs.push_back(v);
                            }
                        } else {
                            v.directsound = false;
                            v.kind = (cls == 4) ? 4 : ((cls == 3) ? 3 : 1);
                            v.duty = 0.5; v.lfsr = 0x7FFF;
                            // CGB note frequency: equal temperament (A4=440 at key 69)
                            v.freqHz = 440.0 * std::pow(2.0, ((double)playedKey - 69.0) / 12.0);
                            v.es = 255; // CGB: sustain held
                            vs.push_back(v);
                        }
                    }
                    else if(cmd == 0xB1) { x.stopped = true; x.pc = 0; break; } // FINE
                    else if(cmd == 0xB2) { x.pc = rom.pointer(x.pc, &ok); }     // GOTO (continue)
                    else if(cmd == 0xB3) { uint32_t d = rom.pointer(x.pc, &ok); x.ret = x.pc + 4; x.inCall = true; x.pc = d; }
                    else if(cmd == 0xB4) { if(x.inCall) { x.pc = x.ret; x.inCall = false; } } // PEND
                    else if(cmd == 0xB5) { uint8_t cnt = rom.u8(x.pc); ++x.pc; uint32_t d = rom.pointer(x.pc, &ok); x.pc += 4;
                        if(x.reptCount == 0) x.reptCount = cnt;
                        if(--x.reptCount > 0) x.pc = d; else x.reptCount = 0; }
                    else if(cmd == 0xBB) { tempoI = 2 * rom.u8(x.pc); ++x.pc; } // TEMPO
                    else if(cmd == 0xBC) { x.keyShift = (int8_t)rom.u8(x.pc); ++x.pc; }
                    else if(cmd == 0xBD) { x.voice = rom.u8(x.pc); ++x.pc; }
                    else if(cmd == 0xBE) { x.vol = rom.u8(x.pc); ++x.pc; }
                    else if(cmd == 0xBF) { x.pan = rom.u8(x.pc); ++x.pc; }
                    else if(cmd == 0xC0) { x.bend = rom.u8(x.pc); ++x.pc; }
                    else if(cmd == 0xC1) { x.bendRange = rom.u8(x.pc); ++x.pc; }
                    else if(cmd == 0xC8) { x.tune = rom.u8(x.pc); ++x.pc; }
                    else if(cmd == 0xCE) { // EOT: release the most recent tied voice of this note
                        std::size_t k = vs.size();
                        while(k > 0) { --k; if(vs[k].active && vs[k].gate < 0) { vs[k].envState = 3; break; } }
                        if(!runArg && rom.u8(x.pc) < 0x80) ++x.pc; // optional key operand
                    }
                    else if(cmd >= 0xB6 && cmd <= 0xCD) {
                        // skip operands of known control cmds we don't synth
                        if(cmd == 0xB9) { x.pc += 3; if(rom.isPointer(rom.u32(x.pc))) x.pc += 4; }
                        else if(cmd == 0xBA || (cmd >= 0xC2 && cmd <= 0xC5) || cmd == 0xC8) { ++x.pc; }
                        else if(cmd == 0xCD) { x.pc += 2; }
                    }
                    if(runArg && x.pc == 0) break;
                }
                ++t;
            }
        }

        // ── step envelopes once per frame ──
        std::size_t vi = 0;
        while(vi < vs.size())
        {
            Voice &v = vs[vi];
            if(v.active)
            {
                if(v.directsound)
                {
                    if(v.envState == 0) { v.env += v.ea; if(v.env >= 255) { v.env = 255; v.envState = 1; } }
                    else if(v.envState == 1) { v.env = v.env * v.ed / 256.0; if(v.env <= v.es) { v.env = v.es; v.envState = 2; } }
                    else if(v.envState == 3) { v.env = v.env * v.er / 256.0; if(v.env <= 1) { v.env = 0; v.envState = 4; v.active = false; } }
                }
                else
                {
                    if(v.envState == 0) { v.env = 255; v.envState = 2; }
                    else if(v.envState == 3) { v.env -= 32; if(v.env <= 0) { v.env = 0; v.active = false; } }
                }
            }
            ++vi;
        }

        // ── render this frame's output samples ──
        int s = 0;
        while(s < frameSamp && si < maxS)
        {
            float l = 0.0f, r = 0.0f;
            std::size_t k = 0;
            while(k < vs.size())
            {
                Voice &v = vs[k];
                if(v.active)
                {
                    double smp = 0.0;
                    if(v.directsound)
                    {
                        if(v.pos >= v.len) { if(v.loop) v.pos = v.loopStart + std::fmod(v.pos - v.len, (double)(v.len - v.loopStart > 0 ? v.len - v.loopStart : 1)); else { v.active = false; } }
                        if(v.active) {
                            const uint32_t ip = (uint32_t)v.pos;
                            const double frac = v.pos - ip;
                            const double s0 = (int8_t)rom.u8(v.dataOff + ip);
                            const double s1 = (int8_t)rom.u8(v.dataOff + (ip + 1 < v.len ? ip + 1 : ip));
                            smp = ((1.0 - frac) * s0 + frac * s1) / 128.0;   // linear interp
                            v.pos += v.freqHz / R;
                        }
                    }
                    else if(v.kind == 4) { v.lfsr = (v.lfsr >> 1) ^ ((v.lfsr & 1) ? 0xB400 : 0); smp = (v.lfsr & 1) ? 0.3 : -0.3; }
                    else { v.phase += v.freqHz / R; if(v.phase >= 1.0) v.phase -= 1.0; smp = (v.phase < v.duty) ? 0.25 : -0.25; }
                    if(v.active) {
                        const double g = (v.env / 255.0) * (v.vel / 127.0) * 0.5;
                        l += (float)(smp * (v.volL / 127.0) * g);
                        r += (float)(smp * (v.volR / 127.0) * g);
                    }
                }
                ++k;
            }
            out[(std::size_t)si * 2] = l; out[(std::size_t)si * 2 + 1] = r;
            ++si; ++s;
        }
        // compact dead voices occasionally
        if(vs.size() > 64) { std::vector<Voice> live; std::size_t z = 0; while(z < vs.size()) { if(vs[z].active) live.push_back(vs[z]); ++z; } vs.swap(live); }

        // stop early if nothing is playing and all tracks stopped
        bool anyTrack = false; t = 0; while(t < trackCount) { if(!tk[t].stopped) anyTrack = true; ++t; }
        bool anyVoice = false; vi = 0; while(vi < vs.size()) { if(vs[vi].active) anyVoice = true; ++vi; }
        if(!anyTrack && !anyVoice) break;
    }
    out.resize((std::size_t)si * 2);
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

    QStringList args;
    args << "-y" << "-i" << wav << "-c:a" << "libopus" << "-b:a" << "96k" << QString::fromStdString(outPath);
    QProcess p; p.start("ffmpeg", args);
    const bool fin = p.waitForFinished(60000);
    QFile::remove(wav);
    return fin && p.exitCode() == 0 && QFile::exists(QString::fromStdString(outPath));
}
