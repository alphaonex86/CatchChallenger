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

// Engine that backs a 4-char game code; *ok=false for a non-Gen3 code.
static Engine engineForCode(const std::string &code, bool *ok)
{
    *ok=true;
    if(code=="BPRE" || code=="BPGE")
        return Engine::Frlg;
    if(code=="BPEE" || code=="AXVE" || code=="AXPE")
        return Engine::Rse;
    *ok=false;
    return Engine::Frlg;
}

// Derive a datapack code for a non-canonical hack from its file name:
// the first lowercase [a-z0-9] token that is not a generic word ("pokemon",
// "version", ...).  "Pokemon Glazed.gba" -> "glazed", "HnS v1.2.1.gba" -> "hns".
// The result matches CATCHCHALLENGER_CHECK_(MAIN|SUB)DATAPACKCODE ("^[a-z0-9]+$").
static std::string slugFromPath(const std::string &romPath)
{
    std::string base=romPath;
    size_t slash=base.find_last_of("/\\");
    if(slash!=std::string::npos)
        base=base.substr(slash+1);
    size_t dot=base.find_last_of('.');
    if(dot!=std::string::npos)
        base=base.substr(0,dot);
    // split into lowercase alnum tokens
    std::vector<std::string> tokens;
    std::string cur;
    size_t i=0;
    while(i<=base.size())
    {
        char c=(i<base.size())?base[i]:' ';
        if((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9'))
        {
            if(c>='A'&&c<='Z')
                c=static_cast<char>(c-'A'+'a');
            cur.push_back(c);
        }
        else
        {
            if(!cur.empty())
                tokens.push_back(cur);
            cur.clear();
        }
        i++;
    }
    size_t t=0;
    while(t<tokens.size())
    {
        const std::string &tk=tokens[t];
        // skip generic words and pure version tokens (start with a digit)
        if(tk!="pokemon" && tk!="version" && tk!="the" && !(tk[0]>='0'&&tk[0]<='9'))
            return tk;
        t++;
    }
    return tokens.empty()?std::string("rom"):tokens.front();
}

GameInfo GameInfo::detect(const std::vector<uint8_t> &rom, const std::string &romPath)
{
    GameInfo info;
    if(rom.size()<0xC0)
        return info;
    std::string code(reinterpret_cast<const char *>(&rom[0xAC]),4);
    uint8_t version=rom[0xBC];
    info.code=code;
    info.version=version;
    // A canonical retail ROM is exactly 16MiB; hacks here are expanded (32MiB) or
    // relocate their tables, so size gates "use the hardcoded offsets" vs "treat
    // as a standalone hack and scan".
    const bool is16MB=(rom.size()==0x1000000u);
    // Per-game table.  Five canonical bases are wired up; siblings that share a
    // region with a base are emitted as sub overlays (mainCode+subCode).
    if(code=="BPRE" && is16MB) // FireRed
    {
        info.valid=true;
        info.label="firered";
        info.mainCode="firered";
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
    else if(code=="AXVE" && is16MB) // Ruby
    {
        info.valid=true;
        info.label="ruby";
        info.mainCode="ruby";
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
    else if(code=="BPGE" && is16MB) // LeafGreen — sub overlay of FireRed (Kanto)
    {
        // Sibling sub: only the sub-overlay needs map decode + naming + wild, so
        // only those tables are wired (verified by signature scan, 2026-06-05).
        // FRLG engine constants are shared with FireRed.  No tmx/tileset/skins are
        // written for a sub, so owGfx/trainers/itemNames/doorTable/animWater stay 0.
        info.valid=true;
        info.label="leafgreen";
        info.mainCode="firered";
        info.subCode="leafgreen";
        info.engine=Engine::Frlg;
        info.mapGroupsOffset=0x3526F8;
        info.tilesInPrimary=640;
        info.metatilesInPrimary=640;
        info.palettesInPrimary=7;
        info.region="kanto";
        info.mapNameTable=0x3F19F8;
        info.mapNameStride=4;
        info.mapNameField=0;
        info.mapNameMinSid=0x58;
        info.mapNameMaxSid=0xC4;
        info.wildTable=0x3C9B64;
        info.speciesNames=0x245F2C;
        // Sevii split matches FireRed so sub paths land in the same region folders.
        info.region2="sevii-islands";
        info.region2MinSid=0x8f;
        info.region2MaxSid=0xc3;
    }
    else if(code=="AXPE" && is16MB) // Sapphire — sub overlay of Ruby (Hoenn)
    {
        info.valid=true;
        info.label="sapphire";
        info.mainCode="ruby";
        info.subCode="sapphire";
        info.engine=Engine::Rse;
        info.mapGroupsOffset=0x308530;
        info.tilesInPrimary=512;
        info.metatilesInPrimary=512;
        info.palettesInPrimary=6;
        info.region="hoenn";
        info.mapNameTable=0x3E743C;
        info.mapNameStride=8;
        info.mapNameField=4;
        info.mapNameMinSid=0x00;
        info.mapNameMaxSid=0x56;
        info.wildTable=0x39D2B4;
        info.speciesNames=0x1F7114;
    }
    else if(code=="BPEE" && is16MB) // Emerald — sub overlay of Ruby (Hoenn)
    {
        info.valid=true;
        info.label="emerald";
        info.mainCode="ruby";
        info.subCode="emerald";
        info.engine=Engine::Rse;
        info.mapGroupsOffset=0x486578;
        info.tilesInPrimary=512;
        info.metatilesInPrimary=512;
        info.palettesInPrimary=6;
        info.region="hoenn";
        // Emerald's section names live at a relocated table; the town/route names
        // match Ruby's exactly, so the slugs (and hence sub paths) align with the
        // Ruby main.  Emerald sids start at 0x24 (LITTLEROOT) and run past 0x6B.
        info.mapNameTable=0x5A135C;
        info.mapNameStride=8;
        info.mapNameField=4;
        info.mapNameMinSid=0x24;
        info.mapNameMaxSid=0xD4;
        info.wildTable=0x552D48;
        info.speciesNames=0x3185C8;
    }
    else
    {
        // Not a canonical retail ROM.  If it still rides a known Gen3 engine
        // (a ROM hack: relocated/expanded), emit it as a STANDALONE main and let
        // the decoder locate gMapGroups by signature scan (mapGroupsOffset=0).
        // Metadata tables are unknown for a hack, so they stay 0 — maps+tilesets
        // render, names fall back to generic, no wild/skins/doors.
        bool eok=false;
        Engine eng=engineForCode(code,&eok);
        if(!eok)
        {
            std::cerr << "Unknown ROM code '" << code << "' version " << (int)version
                      << " (not a supported Gen3 engine)" << std::endl;
            return info;
        }
        info.valid=true;
        info.engine=eng;
        info.label=slugFromPath(romPath);
        info.mainCode=info.label;
        info.region=info.label;
        info.mapGroupsOffset=0; // => Decoder::decodeAll scans for gMapGroups
        if(eng==Engine::Frlg)
        {
            info.tilesInPrimary=640;
            info.metatilesInPrimary=640;
            info.palettesInPrimary=7;
        }
        else
        {
            info.tilesInPrimary=512;
            info.metatilesInPrimary=512;
            info.palettesInPrimary=6;
        }
        std::cerr << "ROM '" << code << "' v" << (int)version << " (" << rom.size()/(1024*1024)
                  << "MB) is not canonical -> standalone hack main '" << info.label
                  << "' (gMapGroups will be scanned)" << std::endl;
    }
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
    game_=GameInfo::detect(data_,path);
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

void GbaRom::setTilesetSplit(uint32_t tilesInPrimary, uint32_t metatilesInPrimary, uint32_t palettesInPrimary)
{
    game_.tilesInPrimary=tilesInPrimary;
    game_.metatilesInPrimary=metatilesInPrimary;
    game_.palettesInPrimary=palettesInPrimary;
}

void GbaRom::setMapNameTable(uint32_t table, uint32_t stride, uint32_t field, uint8_t minSid, uint8_t maxSid)
{
    game_.mapNameTable=table;
    game_.mapNameStride=stride;
    game_.mapNameField=field;
    game_.mapNameMinSid=minSid;
    game_.mapNameMaxSid=maxSid;
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
