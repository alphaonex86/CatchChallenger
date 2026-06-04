#include "Gen3Script.hpp"
#include "Gen3Text.hpp"

#include <unordered_set>

static bool textLooksReadable(const GbaRom &rom, uint32_t off);

ScriptResult::ScriptResult() :
    kind(BotKind::None),
    trainerId(0),
    introText(0),
    defeatText(0)
{
}

Gen3Script::Gen3Script(const GbaRom &rom) :
    rom_(rom)
{
}

std::string Gen3Script::speciesName(uint16_t internalId) const
{
    const GameInfo &gi=rom_.game();
    if(gi.speciesNames==0 || internalId==0)
        return std::string();
    return Gen3Text::display(Gen3Text::decode(rom_,gi.speciesNames+static_cast<uint32_t>(internalId)*11,11));
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
    uint32_t esz=(flags & 0x02) ? 16 : 8;
    uint32_t k=0;
    while(k<partySize)
    {
        uint32_t e=partyPtr+k*esz;
        if(e+6>rom_.size())
            break;
        PartyMon mon;
        mon.level=static_cast<uint8_t>(rom_.u16(e+2));
        mon.species=speciesName(rom_.u16(e+4));
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

ScriptResult Gen3Script::classify(uint16_t trainerType, uint32_t scriptOffset) const
{
    ScriptResult r;
    if(scriptOffset==0 || scriptOffset+6>rom_.size())
        return r;
    const GameInfo &gi=rom_.game();
    uint32_t end=scriptOffset+400;
    if(end+6>rom_.size())
        end=rom_.size()-6;

    // Trainer battle (opcode 0x5C <u8 type> <u16 trainerId>).
    if(trainerType!=0)
    {
        uint32_t p=scriptOffset;
        while(p<end)
        {
            if(rom_.u8(p)==0x5C)
            {
                uint16_t tid=rom_.u16(p+2);
                if(tid<gi.trainersCount)
                {
                    r.kind=BotKind::Fight;
                    r.trainerId=tid;
                    // trainerbattle text pointers: pre-battle (intro) and defeat.
                    // Most types: intro@+6, defeat@+10; the no-intro types 3/9:
                    // defeat@+6.  Validate each so a different layout yields none.
                    uint8_t bt=rom_.u8(p+1);
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
            p++;
        }
    }
    // Mart (opcode 0x86 <u32 ptr to item list>).
    {
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
                    r.kind=BotKind::Mart;
                    r.itemIds=items;
                    return r;
                }
            }
            p++;
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
