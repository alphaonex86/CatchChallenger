#include "Gba.hpp"

#include <cstring>
#include <fstream>
#include <iostream>

GameInfo::GameInfo() :
    valid(false),
    version(0),
    engine(Engine::Frlg),
    mapGroupsOffset(0),
    tilesInPrimary(0),
    metatilesInPrimary(0),
    palettesInPrimary(0),
    owGfxPointers(0),
    owGfxCount(0),
    owPalettes(0),
    region(),
    mapNameTable(0),
    mapNameStride(0),
    mapNameField(0),
    mapNameMinSid(0),
    mapNameMaxSid(0),
    wildTable(0),
    speciesNames(0),
    speciesToNatDex(0),
    trainers(0),
    trainersCount(0),
    itemNames(0),
    healSpecial(0),
    animWaterTile(0),
    animWaterTileCount(0),
    animWaterFrames(0),
    animWaterMs(0),
    animWaterArray(0),
    doorTable(0),
    region2(),
    region2MinSid(0),
    region2MaxSid(0)
{
}

std::string GameInfo::regionFor(uint8_t sid) const
{
    if(!region2.empty() && sid>=region2MinSid && sid<=region2MaxSid)
        return region2;
    return region;
}

GameInfo GameInfo::detect(const std::vector<uint8_t> &rom)
{
    GameInfo info;
    if(rom.size()<0xC0)
        return info;
    std::string code(reinterpret_cast<const char *>(&rom[0xAC]),4);
    uint8_t version=rom[0xBC];
    info.code=code;
    info.version=version;
    // Per-game table.  Only the two requested ROMs are wired up; the structure
    // makes it trivial to add LeafGreen/Emerald/Sapphire later (same engines).
    if(code=="BPRE") // FireRed
    {
        info.valid=true;
        info.label="firered";
        info.engine=Engine::Frlg;
        info.mapGroupsOffset=0x352718;
        info.tilesInPrimary=640;
        info.metatilesInPrimary=640;
        info.palettesInPrimary=7;
        info.owGfxPointers=0x39FE20;
        info.owGfxCount=152;
        info.owPalettes=0x3A51C8;
        info.region="kanto";
        info.mapNameTable=0x3F1BBC;
        info.mapNameStride=4;
        info.mapNameField=0;
        info.mapNameMinSid=0x58;
        info.mapNameMaxSid=0xC4;
        info.wildTable=0x3C9D28;
        info.speciesNames=0x245F50;
        info.speciesToNatDex=0x25205E;
        info.trainers=0x23EB38; // gTrainers[0]; 0x23EB60 was entry 1 (off by one 0x28)
        info.trainersCount=743;
        info.leaderClass=84; // TRAINER_CLASS_LEADER (FireRed): the 8 gym leaders
        info.itemNames=0x3DB098;
        info.healSpecial=0x0160;
        info.animWaterTile=508;
        info.animWaterTileCount=4;
        info.animWaterFrames=5;
        info.animWaterMs=133;
        info.animWaterArray=0x3A76D0;
        info.doorTable=0x35B648; // gDoorAnimGraphicsTable (stride 12)
        // Sevii Islands minimap: sections 0x8f (ONE ISLAND) .. 0xc3 (EMBER SPA);
        // Kanto sections (incl. cinnabar-island 0x60, seafoam 0x8b) are below.
        info.region2="sevii-islands";
        info.region2MinSid=0x8f;
        info.region2MaxSid=0xc3;
    }
    else if(code=="AXVE") // Ruby
    {
        info.valid=true;
        info.label="ruby";
        info.engine=Engine::Rse;
        info.mapGroupsOffset=0x3085a0;
        info.tilesInPrimary=512;
        info.metatilesInPrimary=512;
        info.palettesInPrimary=6;
        info.owGfxPointers=0x36DC70;
        info.owGfxCount=218;
        info.owPalettes=0x373794;
        info.region="hoenn";
        info.mapNameTable=0x3E73E0;
        info.mapNameStride=8;
        info.mapNameField=4;
        info.mapNameMinSid=0x00;
        info.mapNameMaxSid=0x56;
        info.wildTable=0x39D46C;
        info.speciesNames=0x1F7184;
        info.speciesToNatDex=0x1FC52E;
        info.trainers=0x1F0514; // gTrainers[0]; 0x1F053C was entry 1 (off by one 0x28)
        info.trainersCount=694;
        info.leaderClass=0xFF; // RSE gym-leader class not yet pinned -> no gym/leader flag
        info.itemNames=0x3C5580;
        info.healSpecial=0x008E;
        info.animWaterTile=508;
        info.animWaterTileCount=4;
        info.animWaterFrames=4;
        info.animWaterMs=267;
        info.animWaterArray=0x376F3C;
        info.doorTable=0x30F9CC; // gDoorAnimGraphicsTable (stride 12)
    }
    else
        std::cerr << "Unknown ROM code '" << code << "' version " << (int)version << std::endl;
    return info;
}

uint16_t GameInfo::behavior(uint32_t attribute) const
{
    if(engine==Engine::Frlg)
        return static_cast<uint16_t>(attribute & 0x1FF);
    return static_cast<uint16_t>(attribute & 0xFF);
}

uint8_t GameInfo::layerType(uint32_t attribute) const
{
    if(engine==Engine::Frlg)
        return static_cast<uint8_t>((attribute & 0x60000000u) >> 29);
    return static_cast<uint8_t>((attribute & 0xF000u) >> 12);
}

uint8_t GameInfo::attributeSize() const
{
    return engine==Engine::Frlg ? 4 : 2;
}

GbaRom::GbaRom()
{
}

bool GbaRom::load(const std::string &path)
{
    std::ifstream f(path,std::ios::binary);
    if(!f)
    {
        std::cerr << "Unable to open ROM: " << path << std::endl;
        return false;
    }
    f.seekg(0,std::ios::end);
    std::streamoff len=f.tellg();
    f.seekg(0,std::ios::beg);
    if(len<=0)
    {
        std::cerr << "Empty ROM: " << path << std::endl;
        return false;
    }
    data_.resize(static_cast<size_t>(len));
    f.read(reinterpret_cast<char *>(data_.data()),len);
    if(!f)
    {
        std::cerr << "Short read on ROM: " << path << std::endl;
        return false;
    }
    game_=GameInfo::detect(data_);
    return game_.valid;
}

const GameInfo &GbaRom::game() const
{
    return game_;
}

bool GbaRom::isValid() const
{
    return game_.valid && !data_.empty();
}

uint8_t GbaRom::u8(uint32_t offset) const
{
    if(offset>=data_.size())
        return 0;
    return data_[offset];
}

uint16_t GbaRom::u16(uint32_t offset) const
{
    if(offset+1>=data_.size())
        return 0;
    return static_cast<uint16_t>(data_[offset] | (data_[offset+1]<<8));
}

uint32_t GbaRom::u32(uint32_t offset) const
{
    if(offset+3>=data_.size())
        return 0;
    return static_cast<uint32_t>(data_[offset]) |
           (static_cast<uint32_t>(data_[offset+1])<<8) |
           (static_cast<uint32_t>(data_[offset+2])<<16) |
           (static_cast<uint32_t>(data_[offset+3])<<24);
}

int32_t GbaRom::s32(uint32_t offset) const
{
    return static_cast<int32_t>(u32(offset));
}

int16_t GbaRom::s16(uint32_t offset) const
{
    return static_cast<int16_t>(u16(offset));
}

bool GbaRom::isPointer(uint32_t value) const
{
    return value>=0x08000000u && value<0x0A000000u &&
           (value-0x08000000u)<data_.size();
}

uint32_t GbaRom::pointer(uint32_t offset, bool *ok) const
{
    uint32_t value=u32(offset);
    if(!isPointer(value))
    {
        if(ok!=nullptr)
            *ok=false;
        return 0;
    }
    if(ok!=nullptr)
        *ok=true;
    return value-0x08000000u;
}

uint32_t GbaRom::size() const
{
    return static_cast<uint32_t>(data_.size());
}

const uint8_t *GbaRom::raw(uint32_t offset, uint32_t length) const
{
    if(static_cast<uint64_t>(offset)+length>data_.size())
        return nullptr;
    return &data_[offset];
}

std::vector<uint8_t> GbaRom::lz77(uint32_t offset) const
{
    std::vector<uint8_t> out;
    if(offset>=data_.size() || data_[offset]!=0x10)
    {
        std::cerr << "lz77: bad header at " << std::hex << offset << std::dec << std::endl;
        return out;
    }
    uint32_t outSize=static_cast<uint32_t>(data_[offset+1]) |
                     (static_cast<uint32_t>(data_[offset+2])<<8) |
                     (static_cast<uint32_t>(data_[offset+3])<<16);
    out.reserve(outSize);
    uint32_t p=offset+4;
    while(out.size()<outSize)
    {
        if(p>=data_.size())
            break;
        uint8_t flags=data_[p];
        p++;
        uint8_t bit=0;
        while(bit<8 && out.size()<outSize)
        {
            if(flags & (0x80>>bit))
            {
                // back reference: 2 bytes -> length (3..18) + displacement
                if(p+1>=data_.size())
                {
                    out.clear();
                    return out;
                }
                uint8_t b0=data_[p];
                uint8_t b1=data_[p+1];
                p+=2;
                uint32_t length=((b0>>4)&0xF)+3;
                uint32_t disp=((static_cast<uint32_t>(b0)&0xF)<<8) | b1;
                if(disp+1>out.size())
                {
                    out.clear();
                    return out;
                }
                size_t start=out.size()-disp-1;
                uint32_t k=0;
                while(k<length && out.size()<outSize)
                {
                    out.push_back(out[start+k]);
                    k++;
                }
            }
            else
            {
                if(p>=data_.size())
                    break;
                out.push_back(data_[p]);
                p++;
            }
            bit++;
        }
    }
    return out;
}
