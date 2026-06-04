#include "Gen3Script.hpp"
#include "Gen3Text.hpp"

ScriptResult::ScriptResult() :
    kind(BotKind::None),
    trainerId(0)
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

uint32_t Gen3Script::signTextOffset(uint32_t scriptOffset) const
{
    if(scriptOffset==0 || scriptOffset+6>rom_.size())
        return 0;
    uint32_t end=scriptOffset+96;
    if(end+6>rom_.size())
        end=rom_.size()-6;
    // A sign script shows its text via loadpointer (0x0F <bank> <u32 ptr>)
    // feeding a callstd msgbox.  Return the first such bank-0 pointer's text
    // file offset.
    uint32_t p=scriptOffset;
    while(p<end)
    {
        if(rom_.u8(p)==0x0F && rom_.u8(p+1)==0x00)
        {
            bool ok=false;
            uint32_t t=rom_.pointer(p+2,&ok);
            if(ok && t+1<rom_.size())
                return t;
        }
        p++;
    }
    return 0;
}
