#include "Gen3Script.hpp"
#include "Gen3Text.hpp"
#include "Decoder.hpp"

#include <unordered_set>

static bool textLooksReadable(const GbaRom &rom, uint32_t off);

std::unordered_map<uint32_t,uint32_t> Gen3Script::battleOwner_;
std::unordered_map<uint32_t,uint32_t> Gen3Script::martOwner_;
std::unordered_map<uint32_t,uint32_t> Gen3Script::wildOwner_;

ScriptResult::ScriptResult() :
    kind(BotKind::None),
    trainerId(0),
    leader(false),
    introText(0),
    defeatText(0),
    matchOffset(0),
    wildLevel(0)
{
}

Gen3Script::Gen3Script(const GbaRom &rom) :
    rom_(rom)
{
}

std::string Gen3Script::itemName(uint16_t id) const
{
    const GameInfo &gi=rom_.game();
    if(gi.itemNames==0)
        return std::string();
    return Gen3Text::display(Gen3Text::decode(rom_,gi.itemNames+static_cast<uint32_t>(id)*44,14));
}

std::vector<PartyMon> Gen3Script::party(uint16_t trainerId) const
{
    std::vector<PartyMon> out;
    const GameInfo &gi=rom_.game();
    if(gi.trainers==0 || trainerId>=gi.trainersCount)
        return out;
    uint32_t off=gi.trainers+static_cast<uint32_t>(trainerId)*0x28;
    uint8_t flags=rom_.u8(off+0x00);
    uint32_t partySize=rom_.u32(off+0x20);
    bool ok=false;
    uint32_t partyPtr=rom_.pointer(off+0x24,&ok);
    if(!ok || partySize<1 || partySize>6)
        return out;
    // Entry size is set by the custom-moveset flag (0x01): default-moves entries
    // are 8 bytes (iv,lvl,species[,item]); custom-moves entries are 16 (the four
    // extra u16 moves).  The held-item flag (0x02) does NOT change the size.
    uint32_t esz=(flags & 0x01) ? 16 : 8;
    uint32_t k=0;
    while(k<partySize)
    {
        uint32_t e=partyPtr+k*esz;
        if(e+6>rom_.size())
            break;
        PartyMon mon;
        mon.level=static_cast<uint8_t>(rom_.u16(e+2));
        mon.species=Gen3Text::speciesName(rom_,rom_.u16(e+4));
        if(!mon.species.empty())
            out.push_back(mon);
        k++;
    }
    return out;
}

std::string Gen3Script::trainerName(uint16_t trainerId) const
{
    const GameInfo &gi=rom_.game();
    if(gi.trainers==0 || trainerId>=gi.trainersCount)
        return std::string();
    // name[12] at gTrainers[id]+0x04, ALL-CAPS in the ROM -> Title Case.
    return Gen3Text::display(Gen3Text::decode(rom_,gi.trainers+static_cast<uint32_t>(trainerId)*0x28+0x04,12));
}

bool Gen3Script::looksLikeItemList(uint32_t off, std::vector<uint16_t> &items) const
{
    items.clear();
    if(off==0 || off+2>rom_.size())
        return false;
    uint32_t p=off;
    int n=0;
    while(n<60 && p+2<=rom_.size())
    {
        uint16_t id=rom_.u16(p);
        if(id==0)
            break;
        if(id>600)
            return false; // implausible item id
        items.push_back(id);
        p+=2;
        n++;
    }
    return !items.empty() && items.size()<=50;
}

// File offset of the first valid trainerbattle command in [scriptOffset, +400),
// validated structurally so non-trainer script bytes don't false-match:
//   * battle type < 10 and the reserved local_id word (always 0 for single
//     battles, which every trainer here is) is 0;
//   * trainer id in range with a non-empty party whose every level is 1..100;
//   * the intro/defeat pointer at +6 points to readable text.
uint32_t Gen3Script::findBattle(uint32_t scriptOffset, uint8_t *outType, uint16_t *outTid) const
{
    if(scriptOffset==0 || scriptOffset+6>rom_.size())
        return 0;
    const GameInfo &gi=rom_.game();
    uint32_t end=scriptOffset+400;
    if(end+6>rom_.size())
        end=rom_.size()-6;
    uint32_t p=scriptOffset;
    while(p<end)
    {
        if(rom_.u8(p)==0x5C)
        {
            uint8_t bt=rom_.u8(p+1);
            uint16_t tid=rom_.u16(p+2);
            uint16_t reserved=rom_.u16(p+4);
            bool pok=false;
            uint32_t ip=rom_.pointer(p+6,&pok);
            bool partyOk=false;
            if(tid<gi.trainersCount)
            {
                std::vector<PartyMon> pty=party(tid);
                partyOk=!pty.empty();
                size_t i=0;
                while(i<pty.size())
                {
                    if(pty[i].level<1 || pty[i].level>100)
                        partyOk=false;
                    i++;
                }
            }
            if(bt<10 && reserved==0 && tid<gi.trainersCount && pok && textLooksReadable(rom_,ip) && partyOk)
            {
                if(outType!=nullptr)
                    *outType=bt;
                if(outTid!=nullptr)
                    *outTid=tid;
                return p;
            }
        }
        p++;
    }
    return 0;
}

uint32_t Gen3Script::findMart(uint32_t scriptOffset, std::vector<uint16_t> *outItems) const
{
    if(scriptOffset==0 || scriptOffset+6>rom_.size())
        return 0;
    uint32_t end=scriptOffset+400;
    if(end+6>rom_.size())
        end=rom_.size()-6;
    uint32_t p=scriptOffset;
    while(p<end)
    {
        if(rom_.u8(p)==0x86)
        {
            bool ok=false;
            uint32_t lp=rom_.pointer(p+1,&ok);
            std::vector<uint16_t> items;
            if(ok && looksLikeItemList(lp,items))
            {
                if(outItems!=nullptr)
                    *outItems=items;
                return p;
            }
        }
        p++;
    }
    return 0;
}

// Validated structurally like findBattle: a real (alphabetic) species name at a
// plausible Gen3 internal id, level 1..100, plausible held-item id — so script
// data / text bytes that happen to read 0xB6 don't false-match.
uint32_t Gen3Script::findWild(uint32_t scriptOffset, std::string *outSpecies, uint8_t *outLevel) const
{
    if(scriptOffset==0 || scriptOffset+6>rom_.size())
        return 0;
    uint32_t end=scriptOffset+400;
    if(end+6>rom_.size())
        end=rom_.size()-6;
    uint32_t p=scriptOffset;
    while(p<end)
    {
        if(rom_.u8(p)==0xB6)
        {
            uint16_t species=rom_.u16(p+1);
            uint8_t level=rom_.u8(p+3);
            uint16_t item=rom_.u16(p+4);
            if(species>=1 && species<=411 && level>=1 && level<=100 && item<=600)
            {
                std::string name=Gen3Text::speciesName(rom_,species);
                if(!name.empty())
                {
                    if(outSpecies!=nullptr)
                        *outSpecies=name;
                    if(outLevel!=nullptr)
                        *outLevel=level;
                    return p;
                }
            }
        }
        p++;
    }
    return 0;
}

void Gen3Script::indexBattleOwners(const GbaRom &rom, const std::vector<DecodedMap> &maps)
{
    // For each trainerbattle / mart command, the owning NPC is the one whose
    // script starts closest before it (smallest delta).  Any other NPC that
    // reaches the same command does so only because the 400-byte scan overran its
    // own, ended, script — classify() rejects those, so a gym keeps a single
    // leader and a mart a single seller (no phantom shop on a gym, no duplicates).
    battleOwner_.clear();
    martOwner_.clear();
    wildOwner_.clear();
    std::unordered_map<uint32_t,uint32_t> bestBattle; // command -> smallest delta seen
    std::unordered_map<uint32_t,uint32_t> bestMart;
    std::unordered_map<uint32_t,uint32_t> bestWild;
    Gen3Script s(rom);
    size_t mi=0;
    while(mi<maps.size())
    {
        const DecodedMap &m=maps[mi];
        size_t ni=0;
        while(ni<m.npcs.size())
        {
            uint32_t sp=m.npcs[ni].scriptPtr;
            if(sp!=0)
            {
                uint32_t cmd=s.findBattle(sp,nullptr,nullptr);
                if(cmd!=0)
                {
                    uint32_t d=cmd-sp;
                    std::unordered_map<uint32_t,uint32_t>::iterator it=bestBattle.find(cmd);
                    if(it==bestBattle.end() || d<it->second)
                    {
                        bestBattle[cmd]=d;
                        battleOwner_[cmd]=sp;
                    }
                }
                uint32_t mcmd=s.findMart(sp,nullptr);
                if(mcmd!=0)
                {
                    uint32_t d=mcmd-sp;
                    std::unordered_map<uint32_t,uint32_t>::iterator it=bestMart.find(mcmd);
                    if(it==bestMart.end() || d<it->second)
                    {
                        bestMart[mcmd]=d;
                        martOwner_[mcmd]=sp;
                    }
                }
                uint32_t wcmd=s.findWild(sp,nullptr,nullptr);
                if(wcmd!=0)
                {
                    uint32_t d=wcmd-sp;
                    std::unordered_map<uint32_t,uint32_t>::iterator it=bestWild.find(wcmd);
                    if(it==bestWild.end() || d<it->second)
                    {
                        bestWild[wcmd]=d;
                        wildOwner_[wcmd]=sp;
                    }
                }
            }
            ni++;
        }
        mi++;
    }
}

ScriptResult Gen3Script::classify(uint16_t trainerType, uint32_t scriptOffset) const
{
    ScriptResult r;
    if(scriptOffset==0 || scriptOffset+6>rom_.size())
        return r;
    const GameInfo &gi=rom_.game();
    uint32_t end=scriptOffset+400;
    if(end+6>rom_.size())
        end=rom_.size()-6;

    // Trainer battle.  Scanned for EVERY NPC (not only sight-trainers) so
    // script-triggered battles — gym leaders especially — are found.  Ownership
    // (indexBattleOwners) ensures only the NPC whose script truly contains the
    // command is the trainer; the rest just overran into it and stay non-fight.
    (void)trainerType;
    {
        uint8_t bt=0;
        uint16_t tid=0;
        uint32_t p=findBattle(scriptOffset,&bt,&tid);
        std::unordered_map<uint32_t,uint32_t>::const_iterator own=battleOwner_.find(p);
        if(p!=0 && (own==battleOwner_.end() || own->second==scriptOffset))
        {
            r.kind=BotKind::Fight;
            r.trainerId=tid;
            r.matchOffset=p;
            if(gi.leaderClass!=0xFF && rom_.u8(gi.trainers+static_cast<uint32_t>(tid)*0x28+0x01)==gi.leaderClass)
                r.leader=true;
            // trainerbattle text pointers: pre-battle (intro) and defeat.
            // Most types: intro@+6, defeat@+10; the no-intro types 3/9:
            // defeat@+6.  Validate each so a different layout yields none.
            bool ok=false;
            if(bt==3 || bt==9)
            {
                uint32_t dt=rom_.pointer(p+6,&ok);
                if(ok && textLooksReadable(rom_,dt))
                    r.defeatText=dt;
            }
            else
            {
                uint32_t it=rom_.pointer(p+6,&ok);
                if(ok && textLooksReadable(rom_,it))
                    r.introText=it;
                uint32_t dt=rom_.pointer(p+10,&ok);
                if(ok && textLooksReadable(rom_,dt))
                    r.defeatText=dt;
            }
            return r;
        }
    }
    // Mart (opcode 0x86 <u32 ptr to item list>).  Ownership keeps a single seller
    // per shop and prevents a gym/other NPC that overran into a mart command from
    // becoming a phantom shop.
    {
        std::vector<uint16_t> items;
        uint32_t p=findMart(scriptOffset,&items);
        std::unordered_map<uint32_t,uint32_t>::const_iterator own=martOwner_.find(p);
        if(p!=0 && (own==martOwner_.end() || own->second==scriptOffset))
        {
            r.kind=BotKind::Mart;
            r.matchOffset=p;
            r.itemIds=items;
            return r;
        }
    }
    // Static one-off monster on the map (setwildbattle 0xB6): Mewtwo in Cerulean
    // Cave, the legendary birds, the sleeping Snorlax, the fake-item Electrode...
    // Ownership keeps the command with the NPC whose script really contains it.
    {
        std::string sp;
        uint8_t lv=0;
        uint32_t p=findWild(scriptOffset,&sp,&lv);
        std::unordered_map<uint32_t,uint32_t>::const_iterator own=wildOwner_.find(p);
        if(p!=0 && (own==wildOwner_.end() || own->second==scriptOffset))
        {
            r.kind=BotKind::WildMon;
            r.matchOffset=p;
            r.wildSpecies=sp;
            r.wildLevel=lv;
            return r;
        }
    }
    // Heal and PC are intentionally NOT classified from scripts: the candidate
    // signals (the "heal" special id, VAR_0x8004 for PC) are generic specials
    // reused across hundreds of unrelated scripts, so detecting them produces
    // mostly false positives.  Better to omit than to emit wrong heal/PC bots.
    (void)gi;
    return r;
}

// True when the bytes at off look like real Gen3 text (has letters, few
// out-of-charmap bytes) rather than a movement script or pointer table — so a
// movement-only NPC stays empty instead of getting garbage.
static bool textLooksReadable(const GbaRom &rom, uint32_t off)
{
    int letters=0,total=0,bad=0;
    uint32_t n=0;
    while(n<20 && off+n<rom.size())
    {
        uint8_t b=rom.u8(off+n);
        if(b==0xFF)
            break;
        total++;
        if(b>=0xBB && b<=0xEE)                                 // A-Z / a-z
            letters++;
        else if(b==0x00 || (b>=0xA1 && b<=0xBA))               // space/digit/punct
            ;
        else if(b==0xFE || b==0xFB || b==0xFA || b==0xFC || b==0xFD) // control
            ;
        else
            bad++;                                             // out-of-charmap
        n++;
    }
    return total>=3 && letters>=2 && bad*4<total;
}

uint32_t Gen3Script::signTextOffset(uint32_t scriptOffset) const
{
    if(scriptOffset==0 || scriptOffset+6>rom_.size())
        return 0;
    // A talking script shows text via loadpointer (0x0F 0x00 <u32 ptr>) into a
    // msgbox.  Walk the script (and the fragments it goto/call-jumps to) and
    // return the first readable text pointer.  The main fragment is fully scanned
    // first, so its dialogue wins; branches only catch text reached past a jump.
    std::vector<uint32_t> stack;
    stack.push_back(scriptOffset);
    std::unordered_set<uint32_t> seen;
    int frags=0;
    while(!stack.empty() && frags<6)
    {
        uint32_t s=stack.back();
        stack.pop_back();
        if(s==0 || s+6>rom_.size() || seen.count(s)!=0)
            continue;
        seen.insert(s);
        frags++;
        uint32_t end=s+220;
        if(end+6>rom_.size())
            end=rom_.size()-6;
        // (1) the first readable loadpointer text in this fragment is the dialogue
        // — a plain pattern search, NOT a command walk, so a data byte that looks
        // like end/goto can't cut the search short.
        uint32_t p=s;
        while(p+6<=end)
        {
            if(rom_.u8(p)==0x0F && rom_.u8(p+1)==0x00)
            {
                bool ok=false;
                uint32_t t=rom_.pointer(p+2,&ok);
                if(ok && t+1<rom_.size() && textLooksReadable(rom_,t))
                    return t;
            }
            p++;
        }
        // (2) no text here: queue the fragment's goto/call jump targets to try,
        // so dialogue reached only past a branch is still found.
        p=s;
        while(p+5<=end)
        {
            uint8_t op=rom_.u8(p);
            if(op==0x04 || op==0x05)                  // call / goto <u32>
            {
                bool ok=false;
                uint32_t tgt=rom_.pointer(p+1,&ok);
                if(ok)
                    stack.push_back(tgt);
            }
            else if(op==0x06 || op==0x07)             // goto_if / call_if <u8><u32>
            {
                bool ok=false;
                uint32_t tgt=rom_.pointer(p+2,&ok);
                if(ok)
                    stack.push_back(tgt);
            }
            p++;
        }
    }
    return 0;
}

int Gen3Script::findItemOf(const GbaRom &rom, uint32_t scriptOffset)
{
    // 1A 00 80 ii ii 1A 01 80 qq qq 09 01 : the whole item-ball script.
    const uint8_t *p=rom.raw(scriptOffset,12);
    if(p==nullptr)
        return -1;
    if(p[0]==0x1A && p[1]==0x00 && p[2]==0x80 &&
       p[5]==0x1A && p[6]==0x01 && p[7]==0x80 &&
       p[10]==0x09 && p[11]==0x01)
        return static_cast<int>(p[3] | (p[4]<<8));
    return -1;
}
