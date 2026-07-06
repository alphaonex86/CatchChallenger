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

// Find the first occurrence of needle in the ROM; returns 0 when not found.
static uint32_t findSig(const GbaRom &rom, const std::vector<uint8_t> &needle)
{
    const uint8_t *base = rom.raw(0, rom.size());
    if(base == NULL || needle.empty())
        return 0;
    const uint8_t *end = base + rom.size() - needle.size();
    const uint8_t *p = base;
    while(p <= end)
    {
        if(p[0] == needle[0] && std::memcmp(p, needle.data(), needle.size()) == 0)
            return (uint32_t)(p - base);
        ++p;
    }
    return 0;
}

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
        std::vector<uint8_t> pound; // P O U N D 0xFF
        const char *s = "POUND";
        std::size_t i = 0; while(s[i]) { pound.push_back((uint8_t)(0xBB + s[i] - 'A')); ++i; }
        pound.push_back(0xFF);
        const uint32_t at = findSig(rom, pound);
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
        std::vector<uint8_t> mb;
        const char *s = "MASTER BALL";
        std::size_t k = 0;
        while(s[k]) { mb.push_back(s[k] == ' ' ? 0x00 : (uint8_t)(0xBB + s[k] - 'A')); ++k; }
        mb.push_back(0xFF);
        const uint32_t at = findSig(rom, mb);
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

    std::cerr << "Gen3Data: gBaseStats=" << std::hex << gBaseStats
              << " gBattleMoves=" << gBattleMoves << " gMoveNames=" << gMoveNames
              << " gEvolutionTable=" << gEvolutionTable << " gTypeEff=" << gTypeEff
              << " gLearnsets=" << gLearnsets << std::dec << std::endl;

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
