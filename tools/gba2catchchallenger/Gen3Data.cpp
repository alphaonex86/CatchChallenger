#include "Gen3Data.hpp"
#include "Gen3Text.hpp"

#include <cstring>
#include <iostream>

Gen3Data::Gen3Data() {}

// Gen3 type ids 0..17 -> lowercase names used as CatchChallenger type slugs.
static const char *kTypeNames[18] = {
    "normal","fighting","flying","poison","ground","rock","bug","ghost","steel",
    "mystery","fire","water","grass","electric","psychic","ice","dragon","dark"
};
const char *Gen3Data::typeName(int t) { return (t >= 0 && t < 18) ? kTypeNames[t] : "normal"; }
int Gen3Data::typeCount() { return 18; }

// Find the next occurrence of needle at/after `from`; returns 0 when not found.
static uint32_t findSig(const GbaRom &rom, const std::vector<uint8_t> &needle, uint32_t from = 0)
{
    const uint8_t *base = rom.raw(0, rom.size());
    if(base == NULL || needle.empty() || from + needle.size() > rom.size())
        return 0;
    const uint8_t *end = base + rom.size() - needle.size();
    const uint8_t *p = base + from;
    while(p <= end)
    {
        if(p[0] == needle[0] && std::memcmp(p, needle.data(), needle.size()) == 0)
            return (uint32_t)(p - base);
        ++p;
    }
    return 0;
}

// Encode plain ASCII (A-Z a-z 0-9 space) to Gen3 text bytes for anchors.
static std::vector<uint8_t> encodeGen3(const char *s, bool terminate = false)
{
    std::vector<uint8_t> v;
    std::size_t i = 0;
    while(s[i])
    {
        const char c = s[i];
        if(c == ' ') v.push_back(0x00);
        else if(c == '\n') v.push_back(0xFE);
        else if(c >= 'A' && c <= 'Z') v.push_back((uint8_t)(0xBB + c - 'A'));
        else if(c >= 'a' && c <= 'z') v.push_back((uint8_t)(0xD5 + c - 'a'));
        else if(c >= '0' && c <= '9') v.push_back((uint8_t)(0xA1 + c - '0'));
        ++i;
    }
    if(terminate)
        v.push_back(0xFF);
    return v;
}

// TM/HM learnset bit for a TM (1-50) / HM (1-8) number: bit = item - ITEM_TM01,
// so TMnn = bit nn-1 and HMnn = bit 49+nn (TM items 289-338, HM items 339-346).
static uint64_t tmBit(int tm) { return (uint64_t)1 << (tm - 1); }
static uint64_t hmBit(int hm) { return (uint64_t)1 << (49 + hm); }

// A u16 learnset entry is (level<<9 | move); 0xFFFF terminates.
static bool looksLikeLearnset(const GbaRom &rom, uint32_t addr)
{
    bool ok = false;
    const uint32_t off = rom.pointer(addr, &ok);
    if(!ok)
        return false;
    int n = 0;
    uint32_t o = off;
    while(n < 40)
    {
        if(o + 2 > rom.size())
            return false;
        const uint16_t e = rom.u16(o);
        if(e == 0xFFFF)
            return n >= 1; // at least one move learnt
        const int move = e & 0x1FF;
        const int level = e >> 9;
        if(move <= 0 || level > 100)
            return false;
        o += 2;
        ++n;
    }
    return false;
}

bool Gen3Data::decode(const GbaRom &rom)
{
    const GameInfo &gi = rom.game();

    // ── locate the tables by anchoring on known values ──
    // gBaseStats: Bulbasaur (species 1): 45 49 49 45 65 65 GRASS(12) POISON(3) 45 64
    std::vector<uint8_t> bulba = {45,49,49,45,65,65,12,3,45,64};
    const uint32_t bulbaAt = findSig(rom, bulba);
    if(bulbaAt == 0)
    {
        std::cerr << "Gen3Data: could not locate gBaseStats" << std::endl;
        return false;
    }
    const uint32_t gBaseStats = bulbaAt - 28;

    // gBattleMoves: Pound (move 1) effect0 pw40 typeNORMAL0 acc100 pp35, then
    // KarateChop pw50, DoubleSlap pw15, CometPunch pw18, MegaPunch pw80.
    uint32_t gBattleMoves = 0;
    {
        std::vector<uint8_t> pound = {0,40,0,100,35};
        const uint8_t *base = rom.raw(0, rom.size());
        const uint8_t *end = base + rom.size() - 5;
        const uint8_t *p = base;
        while(p <= end)
        {
            if(std::memcmp(p, pound.data(), 5) == 0)
            {
                const uint32_t cand = (uint32_t)(p - base) - 12;
                if(rom.u8(cand + 2*12 + 1) == 50 && rom.u8(cand + 3*12 + 1) == 15 &&
                   rom.u8(cand + 4*12 + 1) == 18 && rom.u8(cand + 5*12 + 1) == 80)
                { gBattleMoves = cand; break; }
            }
            ++p;
        }
    }

    // gMoveNames: move 1 = "POUND"
    uint32_t gMoveNames = 0;
    {
        const uint32_t at = findSig(rom, encodeGen3("POUND", true));
        if(at != 0) gMoveNames = at - 13;
    }

    // gEvolutionTable: Bulbasaur slot 0 = {method=4(EVO_LEVEL), param=16, species=2}.
    // Each Evolution is 8 bytes {u16 method, u16 param, u16 species, u16 pad};
    // 5 slots per species => 40 bytes/species.
    uint32_t gEvolutionTable = 0;
    {
        std::vector<uint8_t> evo = {4,0,16,0,2,0};
        const uint32_t at = findSig(rom, evo);
        if(at != 0) gEvolutionTable = at - 40; // species 1 * 5 slots * 8 bytes
    }

    // gItems: auto-located by the "MASTER BALL" name anchor (gi.itemNames is only
    // set for some ROMs).  gItems[1] = Master Ball; record stride 44, name@+0.
    uint32_t gItems = gi.itemNames;
    {
        const uint32_t at = findSig(rom, encodeGen3("MASTER BALL", true));
        if(at != 0) gItems = at - 44;
    }

    // gTypeEffectiveness: starts NORMAL>ROCK half(5), NORMAL>STEEL half(5)
    uint32_t gTypeEff = 0;
    {
        std::vector<uint8_t> t = {0,5,5,0,8,5};
        gTypeEff = findSig(rom, t);
    }

    // gLevelUpLearnsets: a pointer array whose entries point at learnset data.
    uint32_t gLearnsets = 0;
    {
        uint32_t o = 0;
        while(o + 4 * 30 < rom.size())
        {
            // species 1, 4, 25 must all look like learnsets
            if(looksLikeLearnset(rom, o + 1*4) && looksLikeLearnset(rom, o + 4*4) &&
               looksLikeLearnset(rom, o + 25*4) && looksLikeLearnset(rom, o + 7*4))
            { gLearnsets = o; break; }
            o += 4;
        }
    }

    // gMoveDescriptions: an array of text pointers indexed [move-1].  Anchor on
    // Pound's flavour text (RSE and FRLG reworded it differently), then find
    // the pointer referencing it and verify the next 3 entries are pointers.
    uint32_t gMoveDesc = 0;
    {
        static const char *anchors[2] = {"Pounds the foe", "A physical attack\ndelivered"};
        int ai = 0;
        while(ai < 2 && gMoveDesc == 0)
        {
            uint32_t txt = findSig(rom, encodeGen3(anchors[ai]));
            while(txt != 0 && gMoveDesc == 0)
            {
                const uint32_t val = 0x08000000u + txt;
                uint32_t o = 0;
                while(o + 16 <= rom.size() && gMoveDesc == 0)
                {
                    if(rom.u32(o) == val && rom.isPointer(rom.u32(o + 4)) &&
                       rom.isPointer(rom.u32(o + 8)) && rom.isPointer(rom.u32(o + 12)))
                        gMoveDesc = o;
                    o += 4;
                }
                txt = findSig(rom, encodeGen3(anchors[ai]), txt + 1);
            }
            ++ai;
        }
    }

    // gPokedexEntries[natDex]: categoryName[12] + u16 height + u16 weight +
    // description ptr (+16); the tail differs per engine so the STRIDE is
    // detected via Ivysaur/Venusaur (1.0m/13.0kg, 2.0m/100.0kg) instead of
    // hard-coding it.  Anchor = Bulbasaur ("SEED", 0.7m/6.9kg).
    uint32_t gPokedex = 0;
    uint32_t dexStride = 0;
    if(gi.speciesToNatDex != 0)
    {
        const std::vector<uint8_t> seed = encodeGen3("SEED", true);
        uint32_t at = findSig(rom, seed);
        while(at != 0 && gPokedex == 0)
        {
            if(at + 100 < rom.size() && rom.u16(at + 12) == 7 && rom.u16(at + 14) == 69)
            {
                static const uint32_t strides[3] = {28, 32, 36};
                int si = 0;
                while(si < 3 && gPokedex == 0)
                {
                    const uint32_t st = strides[si];
                    if(rom.u16(at + st + 12) == 10 && rom.u16(at + st + 14) == 130 &&
                       rom.u16(at + 2*st + 12) == 20 && rom.u16(at + 2*st + 14) == 1000)
                    { gPokedex = at - st; dexStride = st; } // entry 0 = the dummy dex slot
                    ++si;
                }
            }
            at = findSig(rom, seed, at + 1);
        }
    }

    // sTMHMMoves (u16[58]: the move each TM01-50/HM01-08 teaches).  Anchor on
    // TM01-04 (Focus Punch, Dragon Claw, Water Pulse, Calm Mind) and verify the
    // HM tail (Cut, Fly, Surf, Strength, Flash, Rock Smash, Waterfall, Dive).
    uint32_t gTmMoves = 0;
    {
        const std::vector<uint8_t> a = {0x08,0x01,0x51,0x01,0x60,0x01,0x5B,0x01};
        static const uint16_t hmTail[8] = {15,19,57,70,148,249,127,291};
        uint32_t at = findSig(rom, a);
        while(at != 0 && gTmMoves == 0)
        {
            bool okhm = (at + 58*2 <= rom.size());
            int k = 0;
            while(okhm && k < 8)
            {
                if(rom.u16(at + (uint32_t)(50 + k)*2) != hmTail[k]) okhm = false;
                ++k;
            }
            if(okhm)
                gTmMoves = at;
            else
                at = findSig(rom, a, at + 1);
        }
    }

    // gTMHMLearnsets (u64 bitmask per species, bit = TM/HM item - ITEM_TM01).
    // Anchor on {species0 = 0, species1 = Bulbasaur's mask} — the Bulbasaur
    // TM/HM compatibility is identical across the five retail games.
    uint32_t gTmLearn = 0;
    if(gTmMoves != 0)
    {
        const uint64_t bulba =
            tmBit(6)|tmBit(9)|tmBit(10)|tmBit(11)|tmBit(17)|tmBit(19)|tmBit(21)|
            tmBit(22)|tmBit(27)|tmBit(32)|tmBit(36)|tmBit(42)|tmBit(43)|tmBit(44)|
            tmBit(45)|hmBit(1)|hmBit(4)|hmBit(5)|hmBit(6);
        std::vector<uint8_t> sig(16, 0);
        int b = 0;
        while(b < 8) { sig[(std::size_t)8 + b] = (uint8_t)(bulba >> (8*b)); ++b; }
        const uint32_t at = findSig(rom, sig);
        if(at != 0)
            gTmLearn = at;
    }

    std::cerr << "Gen3Data: gBaseStats=" << std::hex << gBaseStats
              << " gBattleMoves=" << gBattleMoves << " gMoveNames=" << gMoveNames
              << " gEvolutionTable=" << gEvolutionTable << " gTypeEff=" << gTypeEff
              << " gLearnsets=" << gLearnsets << " gMoveDesc=" << gMoveDesc
              << " gPokedex=" << gPokedex << "(+" << std::dec << dexStride << std::hex << ")"
              << " gTmMoves=" << gTmMoves << " gTmLearn=" << gTmLearn
              << std::dec << std::endl;

    // ── moves ──
    int moveId = 1;
    while(moveId <= 354 && gBattleMoves != 0)
    {
        const uint32_t o = gBattleMoves + (uint32_t)moveId * 12;
        Gen3Move m;
        m.id = moveId;
        m.effect = rom.u8(o + 0);
        m.power = rom.u8(o + 1);
        m.type = rom.u8(o + 2);
        m.accuracy = rom.u8(o + 3);
        m.pp = rom.u8(o + 4);
        m.priority = (int8_t)rom.u8(o + 7);
        if(gMoveNames != 0)
            m.name = Gen3Text::display(Gen3Text::decode(rom, gMoveNames + (uint32_t)moveId * 13, 13));
        if(m.name.empty())
            m.name = "Move" + std::to_string(moveId);
        if(gMoveDesc != 0)
        {
            bool dok = false;
            const uint32_t dp = rom.pointer(gMoveDesc + (uint32_t)(moveId - 1) * 4, &dok);
            if(dok)
                m.description = Gen3Text::decodeParagraph(rom, dp);
        }
        moves_.push_back(m);
        ++moveId;
    }

    // ── species ──
    int sp = 1;
    while(sp <= 411 && gi.speciesNames != 0)
    {
        std::string name = Gen3Text::speciesName(rom, (uint16_t)sp);
        // skip the unused/glitch internal slots (e.g. 252-276) which decode to a
        // placeholder with no real letters.
        bool good = !name.empty();
        bool hasAlpha = false;
        std::size_t c = 0;
        while(c < name.size())
        {
            const unsigned char ch = (unsigned char)name[c];
            if(ch < 0x20) good = false;
            if((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) hasAlpha = true;
            ++c;
        }
        if(good && hasAlpha && name.find('?') == std::string::npos)
        {
            const uint32_t o = gBaseStats + (uint32_t)sp * 28;
            Gen3Species s;
            s.id = sp;
            s.name = name;
            s.hp = rom.u8(o + 0); s.attack = rom.u8(o + 1); s.defense = rom.u8(o + 2);
            s.speed = rom.u8(o + 3); s.spAttack = rom.u8(o + 4); s.spDefense = rom.u8(o + 5);
            s.type1 = rom.u8(o + 6); s.type2 = rom.u8(o + 7);
            s.catchRate = rom.u8(o + 8); s.baseExp = rom.u8(o + 9);
            s.genderRatio = rom.u8(o + 16); s.eggCycles = rom.u8(o + 17); s.growthRate = rom.u8(o + 19);

            // Pokedex category + flavour text (species -> national dex -> entry)
            if(gPokedex != 0)
            {
                const uint16_t dex = rom.u16(gi.speciesToNatDex + (uint32_t)(sp - 1) * 2);
                if(dex >= 1 && dex <= 386)
                {
                    const uint32_t de = gPokedex + (uint32_t)dex * dexStride;
                    s.kind = Gen3Text::display(Gen3Text::decode(rom, de, 12));
                    bool dok = false;
                    const uint32_t dp = rom.pointer(de + 16, &dok);
                    if(dok)
                        s.description = Gen3Text::decodeParagraph(rom, dp);
                }
            }

            // TM/HM compatibility -> (teaching item, move) pairs
            if(gTmLearn != 0)
            {
                const uint64_t mask = (uint64_t)rom.u32(gTmLearn + (uint32_t)sp * 8) |
                                      ((uint64_t)rom.u32(gTmLearn + (uint32_t)sp * 8 + 4) << 32);
                int bit = 0;
                while(bit < 58)
                {
                    if((mask >> bit) & 1)
                    {
                        const int mv = rom.u16(gTmMoves + (uint32_t)bit * 2);
                        if(mv > 0 && mv <= 354)
                            s.tmLearn.push_back(std::make_pair(289 + bit, mv));
                    }
                    ++bit;
                }
            }

            // learnset
            if(gLearnsets != 0)
            {
                bool ok = false;
                const uint32_t lp = rom.pointer(gLearnsets + (uint32_t)sp * 4, &ok);
                if(ok)
                {
                    uint32_t lo = lp; int n = 0;
                    while(n < 60)
                    {
                        const uint16_t e = rom.u16(lo);
                        if(e == 0xFFFF) break;
                        const int mv = e & 0x1FF;
                        const int lv = e >> 9;
                        if(mv > 0 && mv <= 354)
                            s.learnset.push_back(std::make_pair(lv, mv));
                        lo += 2; ++n;
                    }
                }
            }

            // evolutions: 5 slots of 8 bytes {method,param,species,pad}.
            // method 4 / 8..15 = level (param=level); 7 = item (param=item id);
            // 1..3 friendship, 5..6 trade -> approximated as a level evolution.
            if(gEvolutionTable != 0)
            {
                int slot = 0;
                while(slot < 5)
                {
                    const uint32_t eo = gEvolutionTable + (uint32_t)sp * 40 + (uint32_t)slot * 8;
                    const int method = rom.u16(eo + 0);
                    const int param = rom.u16(eo + 2);
                    const int target = rom.u16(eo + 4);
                    if(method >= 1 && method <= 15 && target > 0 && target <= 411)
                    {
                        const bool item = (method == 7); // EVO_ITEM
                        const int lvl = (param >= 1 && param <= 100) ? param : 36;
                        s.evolutions.push_back(std::make_pair(item ? 2 : 1,
                                               std::make_pair(item ? param : lvl, target)));
                    }
                    ++slot;
                }
            }
            species_.push_back(s);
        }
        ++sp;
    }

    // ── items ──  (stride 44, name@+0, price@+16).  The real Gen3 item table is
    // <= 377 entries with internal unused gaps; iterate up to that cap and skip
    // unused/garbage slots (no letters, or absurd price), so we never over-read.
    int it = 1;
    while(it <= 377 && gItems != 0)
    {
        const uint32_t o = gItems + (uint32_t)it * 44;
        if(o + 44 > rom.size()) break;
        std::string name = Gen3Text::display(Gen3Text::decode(rom, o, 14));
        bool hasAlpha = false;
        std::size_t k = 0;
        while(k < name.size()) { const char ch = name[k]; if((ch>='A'&&ch<='Z')||(ch>='a'&&ch<='z')) hasAlpha = true; ++k; }
        const int price = rom.u16(o + 16);
        if(hasAlpha && price <= 60000)
        {
            Gen3Item gitem;
            gitem.id = it;
            gitem.name = name;
            gitem.price = price;
            bool dok = false;
            const uint32_t dp = rom.pointer(o + 20, &dok); // description ptr
            if(dok)
                gitem.description = Gen3Text::decodeParagraph(rom, dp);
            items_.push_back(gitem);
        }
        ++it;
    }

    // ── type chart ──
    if(gTypeEff != 0)
    {
        uint32_t o = gTypeEff;
        int guard = 0;
        while(guard < 400)
        {
            const int atk = rom.u8(o);
            if(atk == 0xFF) break;        // table terminator
            const int def = rom.u8(o + 1);
            const int mul = rom.u8(o + 2);
            if(atk != 0xFE) // 0xFE = foresight separator, skip the triple
            {
                Gen3TypeMatch tm;
                tm.atk = atk; tm.def = def;
                tm.mult = (mul == 0) ? 0.0 : (mul == 5 ? 0.5 : (mul == 20 ? 2.0 : 1.0));
                if(tm.mult != 1.0 && atk < 18 && def < 18)
                    typeChart_.push_back(tm);
            }
            o += 3; ++guard;
        }
    }

    std::cerr << "Gen3Data: " << species_.size() << " species, " << moves_.size()
              << " moves, " << items_.size() << " items, " << typeChart_.size()
              << " type matchups." << std::endl;
    return !species_.empty() && !moves_.empty();
}
