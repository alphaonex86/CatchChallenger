#include "SpriteRipper.hpp"

#include <QImage>
#include <QString>
#include <QDir>

#include <iostream>

SpriteRipper::SpriteRipper() : frontTable_(0), backTable_(0), paletteTable_(0) {}

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

bool SpriteRipper::locate(const GbaRom &rom)
{
    frontTable_ = findPicTable(rom, 6, 0);                       // {ptr,u16 size,u16 tag}
    paletteTable_ = findPicTable(rom, 4, 0);                     // {ptr,u16 tag,u16 pad}
    // back table: the next front-like table after the front one
    if(frontTable_ != 0)
        backTable_ = findPicTable(rom, 6, frontTable_ + 8);
    std::cerr << "SpriteRipper: front=" << std::hex << frontTable_ << " back=" << backTable_
              << " palette=" << paletteTable_ << std::dec << std::endl;
    return frontTable_ != 0 && paletteTable_ != 0;
}

// Decode a 64x64 4bpp GBA tile image + 16-colour BGR555 palette into ARGB32.
static QImage decode(const std::vector<uint8_t> &tiles, const std::vector<uint8_t> &pal)
{
    QImage img(64, 64, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    if(tiles.size() < 64 * 32 || pal.size() < 32)
        return img;
    QRgb colors[16];
    int i = 0;
    while(i < 16)
    {
        const uint16_t c = (uint16_t)(pal[i * 2] | (pal[i * 2 + 1] << 8));
        const int r = (c & 0x1F) * 255 / 31;
        const int g = ((c >> 5) & 0x1F) * 255 / 31;
        const int b = ((c >> 10) & 0x1F) * 255 / 31;
        colors[i] = (i == 0) ? qRgba(0, 0, 0, 0) : qRgba(r, g, b, 255);
        ++i;
    }
    int tile = 0;
    while(tile < 64) // 8x8 tiles
    {
        const int tx = (tile % 8) * 8;
        const int ty = (tile / 8) * 8;
        int py = 0;
        while(py < 8)
        {
            int px = 0;
            while(px < 8)
            {
                const uint8_t byte = tiles[(std::size_t)tile * 32 + py * 4 + px / 2];
                const int idx = (px & 1) ? (byte >> 4) : (byte & 0xF);
                img.setPixel(tx + px, ty + py, colors[idx]);
                ++px;
            }
            ++py;
        }
        ++tile;
    }
    return img;
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
    front.save(base + "front.png", "PNG");
    back.save(base + "back.png", "PNG");
    front.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation).save(base + "small.png", "PNG");
    return true;
}
