#ifndef GBA2CC_GBAGFX_HPP
#define GBA2CC_GBAGFX_HPP

// Shared GBA graphics primitives: the ONE canonical BGR555 -> ARGB expansion
// (per-channel bit replication (c<<3)|(c>>2)), the 4bpp low-nibble-first tile
// decode, and the opaque-content bounding-box crop.

#include <QImage>

#include <cstddef>
#include <cstdint>
#include <vector>

// Expand one BGR555 colour to opaque ARGB32.
inline uint32_t bgr555ToArgb(uint16_t c)
{
    uint32_t r = c & 0x1F;
    uint32_t g = (c >> 5) & 0x1F;
    uint32_t b = (c >> 10) & 0x1F;
    r = (r << 3) | (r >> 2);
    g = (g << 3) | (g >> 2);
    b = (b << 3) | (b >> 2);
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

// Decode a 4bpp GBA tile image (cols x rows 8x8 tiles, byte = 2 px, low nibble
// = left pixel) with a 16-entry ARGB palette; index 0 stays transparent.
inline QImage decode4bppTiles(const std::vector<uint8_t> &tiles, const uint32_t *palette, int cols, int rows)
{
    QImage img(cols * 8, rows * 8, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    const int nt = cols * rows;
    int tile = 0;
    while(tile < nt && (std::size_t)(tile + 1) * 32 <= tiles.size())
    {
        const int tx = (tile % cols) * 8, ty = (tile / cols) * 8;
        int py = 0;
        while(py < 8)
        {
            int px = 0;
            while(px < 8)
            {
                const uint8_t byte = tiles[(std::size_t)tile * 32 + py * 4 + px / 2];
                const int ci = (px & 1) ? (byte >> 4) : (byte & 0xF);
                if(ci != 0)
                    img.setPixel(tx + px, ty + py, palette[ci]);
                ++px;
            }
            ++py;
        }
        ++tile;
    }
    return img;
}

// Crop to the opaque-alpha bounding box; null image when fully transparent.
inline QImage cropToOpaqueContent(const QImage &src)
{
    QImage img = src.convertToFormat(QImage::Format_ARGB32);
    int minX = img.width(), minY = img.height(), maxX = -1, maxY = -1;
    int y = 0;
    while(y < img.height())
    {
        int x = 0;
        while(x < img.width())
        {
            if(qAlpha(img.pixel(x, y)) > 0)
            {
                if(x < minX) minX = x;
                if(x > maxX) maxX = x;
                if(y < minY) minY = y;
                if(y > maxY) maxY = y;
            }
            x++;
        }
        y++;
    }
    if(maxX < minX || maxY < minY)
        return QImage();
    return img.copy(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

#endif // GBA2CC_GBAGFX_HPP
