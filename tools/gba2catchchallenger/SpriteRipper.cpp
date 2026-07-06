#include "SpriteRipper.hpp"
#include "GbaGfx.hpp"

#include <QImage>
#include <QString>
#include <QDir>

#include <iostream>

SpriteRipper::SpriteRipper() : frontTable_(0), backTable_(0), paletteTable_(0), itemIconTable_(0) {}

// Does the ROM pointer stored at file offset ptrFieldOffset target an LZ77
// header (0x10)?  (GbaRom::pointer reads the pointer AT the given file offset.)
static bool ptrToLz77(const GbaRom &rom, uint32_t ptrFieldOffset)
{
    bool ok = false;
    const uint32_t off = rom.pointer(ptrFieldOffset, &ok);
    return ok && off < rom.size() && rom.u8(off) == 0x10;
}

// Find a tag-incrementing pointer-array (8-byte records: ptr at +0, u16 tag at
// tagOff) whose first 8 entries point at LZ77 data and tag==index.
static uint32_t findPicTable(const GbaRom &rom, uint32_t tagOff, uint32_t startAfter)
{
    uint32_t o = (startAfter + 3) & ~3u;
    while(o + 8 * 8 < rom.size())
    {
        bool ok = true;
        int i = 0;
        while(i < 8 && ok)
        {
            const uint32_t rec = o + (uint32_t)i * 8;
            if(!ptrToLz77(rom, rec) || rom.u16(rec + tagOff) != (uint16_t)i)
                ok = false;
            ++i;
        }
        if(ok)
            return o;
        o += 4;
    }
    return 0;
}

// gItemIconTable: a run of {u32 image->LZ77, u32 palette ptr} pairs.  Several
// shorter coincidental runs exist, so pick the LONGEST (the real ~376-entry one).
static uint32_t findItemIconTable(const GbaRom &rom)
{
    uint32_t best = 0, bestRun = 0;
    uint32_t o = 0;
    while(o + 8 <= rom.size())
    {
        uint32_t run = 0, p = o;
        while(p + 8 <= rom.size())
        {
            bool a = false, b = false;
            const uint32_t img = rom.pointer(p, &a);
            const uint32_t pal = rom.pointer(p + 4, &b);
            if(a && b && img < rom.size() && rom.u8(img) == 0x10)
            { ++run; p += 8; }
            else break;
        }
        if(run > bestRun) { bestRun = run; best = o; }
        o += (run > 0) ? run * 8 : 4;   // skip past a run we already scored
    }
    return bestRun >= 100 ? best : 0;
}

bool SpriteRipper::locate(const GbaRom &rom)
{
    frontTable_ = findPicTable(rom, 6, 0);                       // {ptr,u16 size,u16 tag}
    paletteTable_ = findPicTable(rom, 4, 0);                     // {ptr,u16 tag,u16 pad}
    // back table: the next front-like table after the front one
    if(frontTable_ != 0)
        backTable_ = findPicTable(rom, 6, frontTable_ + 8);
    itemIconTable_ = findItemIconTable(rom);
    std::cerr << "SpriteRipper: front=" << std::hex << frontTable_ << " back=" << backTable_
              << " palette=" << paletteTable_ << " itemIcons=" << itemIconTable_ << std::dec << std::endl;
    return frontTable_ != 0 && paletteTable_ != 0;
}

// Decode a 4bpp GBA tile image (cols x rows tiles) + 16-colour BGR555 palette.
static QImage decodeTiles(const std::vector<uint8_t> &tiles, const std::vector<uint8_t> &pal, int cols, int rows)
{
    if(pal.size() < 32)
    {
        QImage img(cols * 8, rows * 8, QImage::Format_ARGB32);
        img.fill(Qt::transparent);
        return img;
    }
    uint32_t colors[16];
    int i = 0;
    while(i < 16)
    {
        colors[i] = bgr555ToArgb((uint16_t)(pal[i * 2] | (pal[i * 2 + 1] << 8)));
        ++i;
    }
    return decode4bppTiles(tiles, colors, cols, rows);
}

bool SpriteRipper::writeItemIcon(const GbaRom &rom, const std::string &outRoot, int itemId) const
{
    if(itemIconTable_ == 0)
        return false;
    bool a = false, b = false;
    const uint32_t imgPtr = rom.pointer(itemIconTable_ + (uint32_t)itemId * 8, &a);
    const uint32_t palPtr = rom.pointer(itemIconTable_ + (uint32_t)itemId * 8 + 4, &b);
    if(!a || !b)
        return false;
    const std::vector<uint8_t> tiles = rom.lz77(imgPtr);
    std::vector<uint8_t> pal = rom.lz77(palPtr);
    if(pal.size() < 32) {                 // palette may be a raw 32-byte block
        pal.clear();
        const uint8_t *p = rom.raw(palPtr, 32);
        if(p != NULL) { int k = 0; while(k < 32) { pal.push_back(p[k]); ++k; } }
    }
    if(tiles.empty() || pal.size() < 32)
        return false;
    // item icons are 24x24 stored in a 32x32 (4x4 tile) OBJ; decode + crop
    const QImage full = decodeTiles(tiles, pal, 4, 4);
    QImage icon = full.copy(0, 0, 24, 24);
    const std::string dir = outRoot + "/items";
    QDir().mkpath(QString::fromStdString(dir));
    return icon.save(QString::fromStdString(dir) + "/icon-" + QString::number(itemId) + ".png", "PNG");
}

// Decode a 64x64 4bpp GBA tile image + 16-colour BGR555 palette into ARGB32
// (blank when the tile data is partial).
static QImage decode(const std::vector<uint8_t> &tiles, const std::vector<uint8_t> &pal)
{
    if(tiles.size() < 64 * 32)
    {
        QImage img(64, 64, QImage::Format_ARGB32);
        img.fill(Qt::transparent);
        return img;
    }
    return decodeTiles(tiles, pal, 8, 8);
}

bool SpriteRipper::writeSpecies(const GbaRom &rom, const std::string &outRoot, int speciesId) const
{
    if(frontTable_ == 0 || paletteTable_ == 0)
        return false;
    bool ok = false;
    const uint32_t frontPtr = rom.pointer(frontTable_ + (uint32_t)speciesId * 8, &ok);
    if(!ok) return false;
    const uint32_t palPtr = rom.pointer(paletteTable_ + (uint32_t)speciesId * 8, &ok);
    if(!ok) return false;

    const std::vector<uint8_t> tiles = rom.lz77(frontPtr);
    const std::vector<uint8_t> pal = rom.lz77(palPtr);
    if(tiles.empty() || pal.empty())
        return false;
    const QImage front = decode(tiles, pal);

    QImage back;
    if(backTable_ != 0)
    {
        const uint32_t backPtr = rom.pointer(backTable_ + (uint32_t)speciesId * 8, &ok);
        if(ok)
        {
            const std::vector<uint8_t> bt = rom.lz77(backPtr);
            if(!bt.empty())
                back = decode(bt, pal);
        }
    }
    if(back.isNull())
        back = front.mirrored(true, false); // fall back to a flipped front

    const std::string dir = outRoot + "/monsters/" + std::to_string(speciesId);
    QDir().mkpath(QString::fromStdString(dir));
    const QString base = QString::fromStdString(dir) + "/";
    const bool okF = front.save(base + "front.png", "PNG");
    const bool okB = back.save(base + "back.png", "PNG");
    const bool okS = front.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation).save(base + "small.png", "PNG");
    return okF && okB && okS;
}
