#include "OverworldSprite.hpp"
#include "GbaGfx.hpp"

#include <vector>

uint32_t OverworldSprite::findPalette(const GbaRom &rom, uint16_t tag)
{
    const GameInfo &g=rom.game();
    uint32_t o=g.owPalettes;
    int guard=0;
    while(guard<128)
    {
        bool ok=false;
        uint32_t dataPtr=rom.pointer(o+0x00,&ok);
        uint16_t t=rom.u16(o+0x04);
        if(!ok && t==0)
            break; // {NULL,0,0} sentinel
        if(ok && t==tag)
            return dataPtr;
        o+=8;
        guard++;
    }
    return 0;
}

void OverworldSprite::buildPalette(const GbaRom &rom, uint16_t palTag, uint32_t *palette)
{
    uint32_t palOff=findPalette(rom,palTag);
    int i=0;
    while(i<16)
    {
        uint16_t c=(palOff!=0) ? rom.u16(palOff+static_cast<uint32_t>(i)*2) : 0;
        palette[i]=bgr555ToArgb(c);
        i++;
    }
}

// Decode one SpriteFrameImage (4bpp, width x height) to an RGBA image, or a
// null image when the frame entry is not a valid pointer.
QImage OverworldSprite::decodeFrame(const GbaRom &rom, uint32_t imagesPtr, int frameIndex,
                                    int width, int height, const uint32_t *palette)
{
    bool ok=false;
    uint32_t entry=imagesPtr+static_cast<uint32_t>(frameIndex)*8;
    uint32_t dataPtr=rom.pointer(entry+0x00,&ok);
    if(!ok)
        return QImage();
    uint16_t frameSize=rom.u16(entry+0x04);
    uint32_t needed=static_cast<uint32_t>(width)*static_cast<uint32_t>(height)/2;
    std::vector<uint8_t> pixels;
    if(rom.u8(dataPtr)==0x10)
        pixels=rom.lz77(dataPtr);
    else
    {
        const uint8_t *p=rom.raw(dataPtr,frameSize);
        if(p!=nullptr)
            pixels.assign(p,p+frameSize);
    }
    if(pixels.size()<needed)
        pixels.resize(needed,0);
    return decode4bppTiles(pixels,palette,width/8,height/8);
}

QImage OverworldSprite::frameOr(const std::vector<QImage> &frames, int idx, int fallback)
{
    if(idx>=0 && idx<static_cast<int>(frames.size()) && !frames[idx].isNull())
        return frames[idx];
    if(fallback>=0 && fallback<static_cast<int>(frames.size()) && !frames[fallback].isNull())
        return frames[fallback];
    if(!frames.empty() && !frames[0].isNull())
        return frames[0];
    return QImage();
}

// Crop a frame to its opaque content and bottom-centre it inside a 16x24 cell.
QImage OverworldSprite::fitCell(const QImage &frame)
{
    QImage cell(16,24,QImage::Format_ARGB32);
    cell.fill(Qt::transparent);
    if(frame.isNull())
        return cell;
    QImage content=cropToOpaqueContent(frame);
    if(content.isNull())
        return cell;
    // Clamp to the cell and bottom-centre (feet at the bottom edge).
    if(content.width()>16 || content.height()>24)
        content=content.copy((content.width()-16)/2,content.height()>24 ? content.height()-24 : 0,
                             content.width()>16?16:content.width(),content.height()>24?24:content.height());
    int dx=(16-content.width())/2;
    int dy=24-content.height();
    int cy=0;
    while(cy<content.height())
    {
        int cx=0;
        while(cx<content.width())
        {
            QRgb p=content.pixel(cx,cy);
            if(qAlpha(p)>0)
                cell.setPixel(dx+cx,dy+cy,p);
            cx++;
        }
        cy++;
    }
    return cell;
}

static void pasteCell(QImage &sheet, int col, int row, const QImage &cell)
{
    int ox=col*16;
    int oy=row*24;
    int y=0;
    while(y<24)
    {
        int x=0;
        while(x<16)
        {
            QRgb p=cell.pixel(x,y);
            if(qAlpha(p)>0)
                sheet.setPixel(ox+x,oy+y,p);
            x++;
        }
        y++;
    }
}

QImage OverworldSprite::renderStatic(const GbaRom &rom, uint8_t graphicsId)
{
    const GameInfo &g=rom.game();
    if(g.owGfxPointers==0 || graphicsId>=g.owGfxCount)
        return QImage();
    bool ok=false;
    uint32_t infoPtr=rom.pointer(g.owGfxPointers+static_cast<uint32_t>(graphicsId)*4,&ok);
    if(!ok)
        return QImage();
    int width=rom.s16(infoPtr+0x08);
    int height=rom.s16(infoPtr+0x0A);
    if(width<8 || height<8 || width>64 || height>64)
        return QImage();
    uint16_t palTag=rom.u16(infoPtr+0x02);
    uint32_t imagesPtr=rom.pointer(infoPtr+0x1C,&ok);
    if(!ok)
        return QImage();
    uint32_t palette[16];
    buildPalette(rom,palTag,palette);
    return decodeFrame(rom,imagesPtr,0,width,height,palette);
}

QImage OverworldSprite::render(const GbaRom &rom, uint8_t graphicsId)
{
    const GameInfo &g=rom.game();
    if(g.owGfxPointers==0 || graphicsId>=g.owGfxCount)
        return QImage();
    bool ok=false;
    uint32_t infoPtr=rom.pointer(g.owGfxPointers+static_cast<uint32_t>(graphicsId)*4,&ok);
    if(!ok)
        return QImage();
    int width=rom.s16(infoPtr+0x08);
    int height=rom.s16(infoPtr+0x0A);
    // Only standard 16x32 walking characters become bots.  Items (16x16), big
    // decorations (32x32+) and other object-events are not people and would
    // render as nonsense walking sheets, so they are rejected here.
    if(width!=16 || height!=32)
        return QImage();
    uint16_t palTag=rom.u16(infoPtr+0x02);
    uint32_t imagesPtr=rom.pointer(infoPtr+0x1C,&ok);
    if(!ok)
        return QImage();

    uint32_t palette[16];
    buildPalette(rom,palTag,palette);

    // Decode the first up-to-9 frames (standard walking layout); absent frames
    // stay null and are filled by the per-direction standing fallback.
    std::vector<QImage> frames;
    int fi=0;
    while(fi<9)
    {
        frames.push_back(decodeFrame(rom,imagesPtr,fi,width,height,palette));
        fi++;
    }
    QImage down=frameOr(frames,0,0);
    if(down.isNull())
        return QImage();
    // A real character leaves margins/transparent space; a solid block that
    // happens to be 16x32 (a sign, a filled object) does not.  Reject when too
    // few transparent pixels, or when the figure is too small/too sparse.
    int transparent=0,opaque=0;
    int yy=0;
    while(yy<down.height())
    {
        int xx=0;
        while(xx<down.width())
        {
            if(qAlpha(down.pixel(xx,yy))>0)
                opaque++;
            else
                transparent++;
            xx++;
        }
        yy++;
    }
    int total=down.width()*down.height();
    if(transparent*100<total*20)   // < 20% transparent -> solid object, not a character
        return QImage();
    if(opaque*100<total*12)        // almost empty -> not a usable sprite
        return QImage();

    // Gen3 standard frame layout: 0 down, 1 up, 2 side(west), 3/4 down-walk,
    // 5/6 up-walk, 7/8 side-walk.  East = west h-flipped.
    QImage sideStand=frameOr(frames,2,0);
    QImage sideWalkA=frameOr(frames,7,2);
    QImage sideWalkB=frameOr(frames,8,2);

    QImage sheet(48,96,QImage::Format_ARGB32);
    sheet.fill(Qt::transparent);
    // row 0 = UP, row 1 = RIGHT, row 2 = DOWN, row 3 = LEFT; col 1 = standing.
    pasteCell(sheet,0,0,fitCell(frameOr(frames,5,1)));
    pasteCell(sheet,1,0,fitCell(frameOr(frames,1,1)));
    pasteCell(sheet,2,0,fitCell(frameOr(frames,6,1)));
    pasteCell(sheet,0,1,fitCell(sideWalkA.mirrored(true,false)));
    pasteCell(sheet,1,1,fitCell(sideStand.mirrored(true,false)));
    pasteCell(sheet,2,1,fitCell(sideWalkB.mirrored(true,false)));
    pasteCell(sheet,0,2,fitCell(frameOr(frames,3,0)));
    pasteCell(sheet,1,2,fitCell(frameOr(frames,0,0)));
    pasteCell(sheet,2,2,fitCell(frameOr(frames,4,0)));
    pasteCell(sheet,0,3,fitCell(sideWalkA));
    pasteCell(sheet,1,3,fitCell(sideStand));
    pasteCell(sheet,2,3,fitCell(sideWalkB));
    return sheet;
}
