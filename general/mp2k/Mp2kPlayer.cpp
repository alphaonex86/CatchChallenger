#include "Mp2kPlayer.hpp"

#include <cmath>
#include <cstring>

namespace CatchChallenger {

// ── note-length / wait table (49 entries) ──
static const uint8_t kClock[49] = {
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
    28,30,32,36,40,42,44,48,52,54,56,60,64,66,68,72,76,78,80,84,88,90,92,96 };

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
    uint32_t dataOff, len, loopStart; bool loop;
    double freqHz, pos;
    double phase, duty; uint32_t lfsr;
    int envState;           // 0 atk 1 dec 2 sus 3 rel 4 dead
    double env; uint8_t ea, ed, es, er;
    bool directsound;
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

// The interpreter+mixer.  Reads ONLY through `src`, so it serves both the ROM
// (converter) and a blob (client) with identical output.
void mp2kRenderSong(const Mp2kSource &src, uint32_t h,
                    int R, double seconds, std::vector<float> &out)
{
    out.clear();
    bool ok = false;
    const int trackCount = src.u8(h);
    if(trackCount < 1 || trackCount > 16) return;
    const uint32_t voicegroup = src.pointer(h + 4, &ok);
    if(!ok) return;

    std::vector<Trk> tk((std::size_t)trackCount);
    int t = 0;
    while(t < trackCount)
    {
        Trk &x = tk[t];
        std::memset(&x, 0, sizeof(Trk));
        x.pc = src.pointer(h + 8 + (uint32_t)t * 4, &ok);
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

    while(si < maxS)
    {
        // ── frame logic (60 Hz): advance ticks per tempo ──
        tempoC += tempoI;
        while(tempoC >= 150)
        {
            tempoC -= 150;
            std::size_t vi = 0;
            while(vi < vs.size())
            {
                if(vs[vi].active && vs[vi].gate > 0)
                {
                    if(--vs[vi].gate == 0) vs[vi].envState = 3; // release
                }
                ++vi;
            }
            t = 0;
            while(t < trackCount)
            {
                Trk &x = tk[t];
                if(x.stopped) { ++t; continue; }
                if(x.wait > 0) { --x.wait; if(x.wait > 0) { ++t; continue; } }
                int guard = 0;
                while(guard++ < 5000 && !x.stopped && x.wait == 0)
                {
                    uint8_t cmd = src.u8(x.pc);
                    bool runArg = false;
                    if(cmd < 0x80) { cmd = x.running; runArg = true; }
                    else { ++x.pc; if(cmd >= 0xBD) x.running = cmd; }

                    if(cmd >= 0x80 && cmd <= 0xB0)        // WAIT
                    { x.wait = kClock[cmd - 0x80]; break; }
                    else if(cmd >= 0xCF)                  // NOTE
                    {
                        int gate = kClock[cmd - 0xCF];    // 0 (TIE) = held
                        uint8_t b0 = src.u8(x.pc);
                        if(b0 < 0x80) { x.key = b0; ++x.pc;
                            uint8_t b1 = src.u8(x.pc);
                            if(b1 < 0x80) { x.vel = b1; ++x.pc;
                                uint8_t b2 = src.u8(x.pc);
                                if(b2 < 0x80) { gate += b2; ++x.pc; }
                            }
                        }
                        uint8_t program = x.voice;
                        uint8_t playedKey = (uint8_t)(x.key + x.keyShift);
                        uint32_t ve = voicegroup + (uint32_t)program * 12;
                        uint8_t type = src.u8(ve);
                        uint8_t toneKey = src.u8(ve + 1);
                        uint32_t wavPtr = src.pointer(ve + 4, &ok);
                        uint8_t ea = src.u8(ve + 8), ed = src.u8(ve + 9), es = src.u8(ve + 10), er = src.u8(ve + 11);
                        if(type == 0x40) {                // keysplit
                            uint32_t subVG = src.pointer(ve + 4, &ok);
                            uint32_t tbl = src.pointer(ve + 8, &ok);
                            uint8_t idx = src.u8(tbl + playedKey);
                            uint32_t se = subVG + (uint32_t)idx * 12;
                            type = src.u8(se); toneKey = src.u8(se + 1);
                            wavPtr = src.pointer(se + 4, &ok);
                            ea = src.u8(se + 8); ed = src.u8(se + 9); es = src.u8(se + 10); er = src.u8(se + 11);
                        } else if(type == 0x80) {         // drumkit
                            uint32_t subVG = src.pointer(ve + 4, &ok);
                            uint32_t se = subVG + (uint32_t)playedKey * 12;
                            type = src.u8(se); toneKey = src.u8(se + 1);
                            wavPtr = src.pointer(se + 4, &ok);
                            ea = src.u8(se + 8); ed = src.u8(se + 9); es = src.u8(se + 10); er = src.u8(se + 11);
                            playedKey = toneKey; // drum plays at its own key
                        }
                        const int cls = type & 0x07;
                        Voice v; std::memset(&v, 0, sizeof(Voice));
                        v.active = true; v.gate = (gate == 0) ? -1 : gate; v.vel = x.vel;
                        v.ea = ea; v.ed = ed; v.es = es; v.er = er; v.env = 0; v.envState = 0;
                        const int y = 2 * x.pan - 128;   // -128..126
                        const int vx = x.vol; // 0..127
                        v.volR = ((y + 128) * vx) >> 8;
                        v.volL = ((127 - y) * vx) >> 8;
                        if((type & 0x07) == 0) {
                            v.directsound = true; v.kind = 0;
                            if(ok && wavPtr + 16 <= src.size()) {
                                const uint32_t freq = src.u32(wavPtr + 4);
                                v.loopStart = src.u32(wavPtr + 8);
                                v.len = src.u32(wavPtr + 12);
                                v.dataOff = wavPtr + 16;
                                v.loop = (src.u16(wavPtr + 2) != 0) || v.loopStart < v.len;
                                if(type & 0x08)   // FIX: fixed rate (drums)
                                    v.freqHz = (double)freq / 1024.0;
                                else
                                    v.freqHz = pitchHz((double)freq, playedKey, toneKey, 0, x.bend, x.bendRange, x.tune);
                                if(v.len > 0 && v.len < 4000000 && v.dataOff + v.len <= src.size())
                                    vs.push_back(v);
                            }
                        } else {
                            v.directsound = false;
                            v.kind = (cls == 4) ? 4 : ((cls == 3) ? 3 : 1);
                            v.duty = 0.5; v.lfsr = 0x7FFF;
                            v.freqHz = 440.0 * std::pow(2.0, ((double)playedKey - 69.0) / 12.0);
                            v.es = 255; // CGB: sustain held
                            vs.push_back(v);
                        }
                    }
                    else if(cmd == 0xB1) { x.stopped = true; x.pc = 0; break; } // FINE
                    else if(cmd == 0xB2) { x.pc = src.pointer(x.pc, &ok); }     // GOTO
                    else if(cmd == 0xB3) { uint32_t d = src.pointer(x.pc, &ok); x.ret = x.pc + 4; x.inCall = true; x.pc = d; }
                    else if(cmd == 0xB4) { if(x.inCall) { x.pc = x.ret; x.inCall = false; } } // PEND
                    else if(cmd == 0xB5) { uint8_t cnt = src.u8(x.pc); ++x.pc; uint32_t d = src.pointer(x.pc, &ok); x.pc += 4;
                        if(x.reptCount == 0) x.reptCount = cnt;
                        if(--x.reptCount > 0) x.pc = d; else x.reptCount = 0; }
                    else if(cmd == 0xBB) { tempoI = 2 * src.u8(x.pc); ++x.pc; } // TEMPO
                    else if(cmd == 0xBC) { x.keyShift = (int8_t)src.u8(x.pc); ++x.pc; }
                    else if(cmd == 0xBD) { x.voice = src.u8(x.pc); ++x.pc; }
                    else if(cmd == 0xBE) { x.vol = src.u8(x.pc); ++x.pc; }
                    else if(cmd == 0xBF) { x.pan = src.u8(x.pc); ++x.pc; }
                    else if(cmd == 0xC0) { x.bend = src.u8(x.pc); ++x.pc; }
                    else if(cmd == 0xC1) { x.bendRange = src.u8(x.pc); ++x.pc; }
                    else if(cmd == 0xC8) { x.tune = src.u8(x.pc); ++x.pc; }
                    else if(cmd == 0xCE) { // EOT: release the most recent tied voice
                        std::size_t k = vs.size();
                        while(k > 0) { --k; if(vs[k].active && vs[k].gate < 0) { vs[k].envState = 3; break; } }
                        if(!runArg && src.u8(x.pc) < 0x80) ++x.pc; // optional key operand
                    }
                    else if(cmd >= 0xB6 && cmd <= 0xCD) {
                        if(cmd == 0xB9) { x.pc += 3; if(src.isPointer(src.u32(x.pc))) x.pc += 4; }
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
                            const double s0 = (int8_t)src.u8(v.dataOff + ip);
                            const double s1 = (int8_t)src.u8(v.dataOff + (ip + 1 < v.len ? ip + 1 : ip));
                            smp = ((1.0 - frac) * s0 + frac * s1) / 128.0;
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
        if(vs.size() > 64) { std::vector<Voice> live; std::size_t z = 0; while(z < vs.size()) { if(vs[z].active) live.push_back(vs[z]); ++z; } vs.swap(live); }

        bool anyTrack = false; t = 0; while(t < trackCount) { if(!tk[t].stopped) anyTrack = true; ++t; }
        bool anyVoice = false; vi = 0; while(vi < vs.size()) { if(vs[vi].active) anyVoice = true; ++vi; }
        if(!anyTrack && !anyVoice) break;
    }
    out.resize((std::size_t)si * 2);
}

// ───────────────────────── Recording source ─────────────────────────
Mp2kRecordingSource::Mp2kRecordingSource(const Mp2kSource &base) : base_(base)
{
    touched_.assign(base.size(), false);
}
void Mp2kRecordingSource::mark(uint32_t offset, uint32_t len) const
{
    uint32_t i = 0;
    while(i < len && (offset + i) < touched_.size()) { touched_[offset + i] = true; ++i; }
}
uint8_t Mp2kRecordingSource::u8(uint32_t offset) const { mark(offset, 1); return base_.u8(offset); }
uint16_t Mp2kRecordingSource::u16(uint32_t offset) const { mark(offset, 2); return base_.u16(offset); }
uint32_t Mp2kRecordingSource::u32(uint32_t offset) const { mark(offset, 4); return base_.u32(offset); }
uint32_t Mp2kRecordingSource::pointer(uint32_t offset, bool *ok) const { mark(offset, 4); return base_.pointer(offset, ok); }
bool Mp2kRecordingSource::isPointer(uint32_t value) const { return base_.isPointer(value); }
uint32_t Mp2kRecordingSource::size() const { return base_.size(); }

static void putU32(std::vector<uint8_t> &b, uint32_t v)
{ b.push_back(v & 0xFF); b.push_back((v >> 8) & 0xFF); b.push_back((v >> 16) & 0xFF); b.push_back((v >> 24) & 0xFF); }

void Mp2kRecordingSource::serialize(uint32_t songHeaderOffset, std::vector<uint8_t> &out) const
{
    // Coalesce touched bytes into [off,len) runs.
    std::vector<std::pair<uint32_t,uint32_t> > runs;
    uint32_t i = 0, n = (uint32_t)touched_.size();
    while(i < n)
    {
        if(touched_[i])
        {
            uint32_t start = i;
            while(i < n && touched_[i]) ++i;
            runs.push_back(std::make_pair(start, i - start));
        }
        else ++i;
    }
    out.clear();
    out.push_back('M'); out.push_back('P'); out.push_back('2'); out.push_back('K');
    out.push_back(1); out.push_back(0); out.push_back(0); out.push_back(0); // version + pad
    putU32(out, songHeaderOffset);
    putU32(out, base_.size());
    putU32(out, (uint32_t)runs.size());
    std::size_t ri = 0;
    while(ri < runs.size())
    {
        const uint32_t off = runs[ri].first, len = runs[ri].second;
        putU32(out, off); putU32(out, len);
        uint32_t k = 0;
        while(k < len) { out.push_back(base_.u8(off + k)); ++k; }
        ++ri;
    }
}

// ───────────────────────── Blob source ─────────────────────────
static uint32_t rdU32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

Mp2kBlobSource::Mp2kBlobSource(const uint8_t *blob, size_t len)
    : songHeaderOffset_(0), romSize_(0), valid_(false)
{
    if(blob == NULL || len < 20) return;
    if(!(blob[0] == 'M' && blob[1] == 'P' && blob[2] == '2' && blob[3] == 'K')) return;
    if(blob[4] != 1) return;
    songHeaderOffset_ = rdU32(blob + 8);
    romSize_ = rdU32(blob + 12);
    uint32_t nSeg = rdU32(blob + 16);
    size_t pos = 20;
    uint32_t s = 0;
    while(s < nSeg)
    {
        if(pos + 8 > len) return;
        Seg seg;
        seg.off = rdU32(blob + pos); seg.len = rdU32(blob + pos + 4); pos += 8;
        if(pos + seg.len > len) return;
        seg.data = blob + pos; pos += seg.len;
        segs_.push_back(seg);
        ++s;
    }
    valid_ = true;
}
const Mp2kBlobSource::Seg *Mp2kBlobSource::find(uint32_t offset) const
{
    // segments are written in ascending off order -> binary search
    size_t lo = 0, hi = segs_.size();
    while(lo < hi)
    {
        size_t mid = (lo + hi) / 2;
        const Seg &sg = segs_[mid];
        if(offset < sg.off) hi = mid;
        else if(offset >= sg.off + sg.len) lo = mid + 1;
        else return &sg;
    }
    return NULL;
}
uint8_t Mp2kBlobSource::u8(uint32_t offset) const
{
    const Seg *sg = find(offset);
    return sg ? sg->data[offset - sg->off] : 0;
}
uint16_t Mp2kBlobSource::u16(uint32_t offset) const { return (uint16_t)u8(offset) | ((uint16_t)u8(offset + 1) << 8); }
uint32_t Mp2kBlobSource::u32(uint32_t offset) const
{ return (uint32_t)u8(offset) | ((uint32_t)u8(offset + 1) << 8) | ((uint32_t)u8(offset + 2) << 16) | ((uint32_t)u8(offset + 3) << 24); }
bool Mp2kBlobSource::isPointer(uint32_t value) const
{ return value >= 0x08000000u && value < 0x0A000000u && (value - 0x08000000u) < romSize_; }
uint32_t Mp2kBlobSource::pointer(uint32_t offset, bool *ok) const
{
    uint32_t value = u32(offset);
    if(!isPointer(value)) { if(ok) *ok = false; return 0; }
    if(ok) *ok = true;
    return value - 0x08000000u;
}

bool mp2kRenderBlob(const uint8_t *blob, size_t len, int sampleRate, double seconds, std::vector<float> &out)
{
    Mp2kBlobSource src(blob, len);
    if(!src.valid()) return false;
    mp2kRenderSong(src, src.songHeaderOffset(), sampleRate, seconds, out);
    return !out.empty();
}

} // namespace CatchChallenger
