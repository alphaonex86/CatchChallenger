#include "Wild.hpp"
#include "Gen3Text.hpp"

#include <iostream>

// Standard Gen3 per-slot encounter weights (percent), summing to 100.
// (Slots 8-11 are 4,4,1,1 — a 5,5,4,1 tail sums to 105 and the engine then
// DROPS the whole encounter list: "total luck is not egal to 100".)
static const int kLandWeights[12]={20,20,10,10,10,10,5,5,4,4,1,1};
static const int kWaterWeights[5]={60,30,5,4,1};
// Rock smash uses the water distribution; fishing is 2 old-rod slots (70,30),
// 3 good-rod (60,20,20), 5 super-rod (40,40,15,4,1) — each rod its own list
// (the curated datapack cascades them: waterSuperRod;waterGoodRod;waterOldRod).
static const int kRockWeights[5]={60,30,5,4,1};
static const int kFishWeights[10]={70,30,60,20,20,40,40,15,4,1};

Wild::Wild(const GbaRom &rom) :
    rom_(rom)
{
}

std::vector<WildSlot> Wild::decodeInfo(uint32_t infoPtr, int count, const int *weights)
{
    std::vector<WildSlot> out;
    if(infoPtr==0 || infoPtr+8>rom_.size())
        return out;
    bool ok=false;
    uint32_t arr=rom_.pointer(infoPtr+0x04,&ok);
    if(!ok)
        return out;
    int i=0;
    while(i<count)
    {
        uint32_t e=arr+static_cast<uint32_t>(i)*4;
        if(e+4>rom_.size())
            break;
        WildSlot slot;
        slot.minLevel=rom_.u8(e+0);
        slot.maxLevel=rom_.u8(e+1);
        uint16_t species=rom_.u16(e+2);
        slot.species=Gen3Text::speciesName(rom_,species);
        slot.weight=weights[i];
        out.push_back(slot);
        i++;
    }
    return out;
}

void Wild::build()
{
    const GameInfo &gi=rom_.game();
    if(gi.wildTable==0)
        return;
    uint32_t o=gi.wildTable;
    int guard=0;
    while(guard<2048)
    {
        uint8_t grp=rom_.u8(o+0);
        uint8_t num=rom_.u8(o+1);
        if(grp==0xFF)
            break;
        bool ok=false;
        uint32_t land=rom_.pointer(o+0x04,&ok);
        bool okW=false;
        uint32_t water=rom_.pointer(o+0x08,&okW);
        bool okR=false;
        uint32_t rock=rom_.pointer(o+0x0C,&okR);
        bool okF=false;
        uint32_t fish=rom_.pointer(o+0x10,&okF);
        WildSet set;
        if(ok)
            set.land=decodeInfo(land,12,kLandWeights);
        if(okW)
            set.water=decodeInfo(water,5,kWaterWeights);
        if(okR)
            set.rock=decodeInfo(rock,5,kRockWeights);
        if(okF)
        {
            std::vector<WildSlot> all=decodeInfo(fish,10,kFishWeights);
            size_t f=0;
            while(f<all.size())
            {
                if(f<2)
                    set.rodOld.push_back(all[f]);
                else if(f<5)
                    set.rodGood.push_back(all[f]);
                else
                    set.rodSuper.push_back(all[f]);
                f++;
            }
        }
        if(!set.land.empty() || !set.water.empty() || !set.rock.empty() ||
           !set.rodOld.empty() || !set.rodGood.empty() || !set.rodSuper.empty())
            sets_[static_cast<uint16_t>((grp<<8)|num)]=set;
        o+=0x14;
        guard++;
    }
    std::cout << "Wild: " << sets_.size() << " maps with encounters" << std::endl;
}

const WildSet *Wild::find(uint8_t group, uint8_t map) const
{
    std::unordered_map<uint16_t,WildSet>::const_iterator it=sets_.find(static_cast<uint16_t>((group<<8)|map));
    if(it==sets_.cend())
        return nullptr;
    return &it->second;
}
