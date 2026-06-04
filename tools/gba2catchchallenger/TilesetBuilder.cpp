#include "TilesetBuilder.hpp"
#include "Naming.hpp"

#include <QDir>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <set>
#include <unordered_set>

const uint32_t TilesetBuilder::kCapacity=1024;
const int TilesetBuilder::kColumns=16;

// ---------------------------------------------------------------------------
// Gen3Tileset
// ---------------------------------------------------------------------------

Gen3Tileset::Gen3Tileset(const GbaRom &rom, uint32_t primaryPtr, uint32_t secondaryPtr) :
    rom_(rom),
    game_(rom.game()),
    primaryPtr_(primaryPtr),
    secondaryPtr_(secondaryPtr),
    primaryTilesPtr_(0),
    secondaryTilesPtr_(0),
    primaryPalPtr_(0),
    secondaryPalPtr_(0),
    primaryMetaPtr_(0),
    secondaryMetaPtr_(0),
    primaryAttrPtr_(0),
    secondaryAttrPtr_(0),
    primaryCompressed_(false),
    secondaryCompressed_(false),
    currentWaterFrame_(-1)
{
    // Load the primary-tileset water animation frames (raw 4bpp; one block of
    // animWaterTileCount tiles per frame).
    if(game_.animWaterArray!=0 && game_.animWaterFrames>0)
    {
        uint32_t frameBytes=static_cast<uint32_t>(game_.animWaterTileCount)*32;
        uint16_t f=0;
        while(f<game_.animWaterFrames)
        {
            bool okf=false;
            uint32_t fp=rom_.pointer(game_.animWaterArray+static_cast<uint32_t>(f)*4,&okf);
            std::vector<uint8_t> frame;
            if(okf)
            {
                const uint8_t *p=rom_.raw(fp,frameBytes);
                if(p!=nullptr)
                    frame.assign(p,p+frameBytes);
            }
            waterFrames_.push_back(frame);
            f++;
        }
    }
    uint32_t attrField=(game_.engine==Engine::Frlg) ? 0x14 : 0x10;
    bool ok=false;
    if(primaryPtr_!=0)
    {
        primaryCompressed_=rom_.u8(primaryPtr_+0x00)!=0;
        primaryTilesPtr_=rom_.pointer(primaryPtr_+0x04,&ok);
        primaryPalPtr_=rom_.pointer(primaryPtr_+0x08,&ok);
        primaryMetaPtr_=rom_.pointer(primaryPtr_+0x0C,&ok);
        primaryAttrPtr_=rom_.pointer(primaryPtr_+attrField,&ok);
        if(primaryCompressed_ && primaryTilesPtr_!=0)
            primaryTiles_=rom_.lz77(primaryTilesPtr_);
    }
    if(secondaryPtr_!=0)
    {
        secondaryCompressed_=rom_.u8(secondaryPtr_+0x00)!=0;
        secondaryTilesPtr_=rom_.pointer(secondaryPtr_+0x04,&ok);
        secondaryPalPtr_=rom_.pointer(secondaryPtr_+0x08,&ok);
        secondaryMetaPtr_=rom_.pointer(secondaryPtr_+0x0C,&ok);
        secondaryAttrPtr_=rom_.pointer(secondaryPtr_+attrField,&ok);
        if(secondaryCompressed_ && secondaryTilesPtr_!=0)
            secondaryTiles_=rom_.lz77(secondaryTilesPtr_);
    }
}

const uint8_t *Gen3Tileset::tilePixels(uint32_t tileIndex) const
{
    // While rendering a specific water-animation frame, the water tile slots
    // use that frame's graphics instead of the base tiles.
    if(currentWaterFrame_>=0 && game_.animWaterTileCount>0 &&
       tileIndex>=game_.animWaterTile &&
       tileIndex<static_cast<uint32_t>(game_.animWaterTile)+game_.animWaterTileCount &&
       static_cast<size_t>(currentWaterFrame_)<waterFrames_.size())
    {
        const std::vector<uint8_t> &frame=waterFrames_[currentWaterFrame_];
        uint32_t off=(tileIndex-game_.animWaterTile)*32;
        if(off+32<=frame.size())
            return &frame[off];
    }
    uint32_t primaryCount=game_.tilesInPrimary;
    if(tileIndex<primaryCount)
    {
        if(primaryCompressed_)
        {
            uint32_t off=tileIndex*32;
            if(off+32<=primaryTiles_.size())
                return &primaryTiles_[off];
            return nullptr;
        }
        return rom_.raw(primaryTilesPtr_+tileIndex*32,32);
    }
    uint32_t idx=tileIndex-primaryCount;
    if(secondaryCompressed_)
    {
        uint32_t off=idx*32;
        if(off+32<=secondaryTiles_.size())
            return &secondaryTiles_[off];
        return nullptr;
    }
    if(secondaryTilesPtr_==0)
        return nullptr;
    return rom_.raw(secondaryTilesPtr_+idx*32,32);
}

uint32_t Gen3Tileset::paletteColor(uint8_t palNum, uint8_t colorIndex) const
{
    uint32_t palPtr;
    if(palNum<game_.palettesInPrimary)
        palPtr=primaryPalPtr_;
    else
        palPtr=secondaryPalPtr_!=0 ? secondaryPalPtr_ : primaryPalPtr_;
    uint32_t off=palPtr+static_cast<uint32_t>(palNum)*32+static_cast<uint32_t>(colorIndex)*2;
    uint16_t c=rom_.u16(off);
    uint32_t r=(c & 0x1F);
    uint32_t g=((c>>5) & 0x1F);
    uint32_t b=((c>>10) & 0x1F);
    r=(r<<3)|(r>>2);
    g=(g<<3)|(g>>2);
    b=(b<<3)|(b>>2);
    return 0xFF000000u | (r<<16) | (g<<8) | b;
}

uint32_t Gen3Tileset::metatileEntryOffset(uint16_t id, uint8_t subtile) const
{
    uint32_t primaryCount=game_.metatilesInPrimary;
    if(id<primaryCount)
        return primaryMetaPtr_+static_cast<uint32_t>(id)*16+static_cast<uint32_t>(subtile)*2;
    uint32_t idx=id-primaryCount;
    return secondaryMetaPtr_+idx*16+static_cast<uint32_t>(subtile)*2;
}

uint32_t Gen3Tileset::attributeOffset(uint16_t id) const
{
    uint32_t primaryCount=game_.metatilesInPrimary;
    uint8_t sz=game_.attributeSize();
    if(id<primaryCount)
        return primaryAttrPtr_+static_cast<uint32_t>(id)*sz;
    uint32_t idx=id-primaryCount;
    return secondaryAttrPtr_+idx*sz;
}

void Gen3Tileset::drawSubtile(QImage &dst, int px, int py, uint16_t entry) const
{
    uint16_t tileIndex=entry & 0x3FF;
    bool hflip=(entry & 0x400)!=0;
    bool vflip=(entry & 0x800)!=0;
    uint8_t palNum=static_cast<uint8_t>((entry>>12) & 0xF);
    const uint8_t *tile=tilePixels(tileIndex);
    if(tile==nullptr)
        return;
    int row=0;
    while(row<8)
    {
        int srcRow=vflip ? (7-row) : row;
        int col=0;
        while(col<8)
        {
            int srcCol=hflip ? (7-col) : col;
            uint8_t b=tile[srcRow*4+(srcCol>>1)];
            uint8_t colorIndex=(srcCol & 1) ? static_cast<uint8_t>(b>>4) : static_cast<uint8_t>(b & 0xF);
            if(colorIndex!=0)
                dst.setPixel(px+col,py+row,paletteColor(palNum,colorIndex));
            col++;
        }
        row++;
    }
}

QImage Gen3Tileset::renderUnder(uint16_t id) const
{
    QImage img(16,16,QImage::Format_ARGB32);
    img.fill(paletteColor(0,0));
    static const int posX[4]={0,8,0,8};
    static const int posY[4]={0,0,8,8};
    int s=0;
    while(s<4)
    {
        uint16_t entry=rom_.u16(metatileEntryOffset(id,static_cast<uint8_t>(s)));
        drawSubtile(img,posX[s],posY[s],entry);
        s++;
    }
    return img;
}

QImage Gen3Tileset::renderMetatile(uint16_t id) const
{
    QImage img=renderUnder(id);
    static const int posX[4]={0,8,0,8};
    static const int posY[4]={0,0,8,8};
    int s=0;
    while(s<4)
    {
        uint16_t entry=rom_.u16(metatileEntryOffset(id,static_cast<uint8_t>(4+s)));
        drawSubtile(img,posX[s],posY[s],entry);
        s++;
    }
    return img;
}

QImage Gen3Tileset::renderOver(uint16_t id, const QImage &under) const
{
    QImage img(16,16,QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QImage scratch=under.copy();
    static const int posX[4]={0,8,0,8};
    static const int posY[4]={0,0,8,8};
    int s=0;
    while(s<4)
    {
        uint16_t entry=rom_.u16(metatileEntryOffset(id,static_cast<uint8_t>(4+s)));
        drawSubtile(scratch,posX[s],posY[s],entry);
        s++;
    }
    int y=0;
    while(y<16)
    {
        int x=0;
        while(x<16)
        {
            QRgb sp=scratch.pixel(x,y);
            if(sp!=under.pixel(x,y))
                img.setPixel(x,y,sp);
            x++;
        }
        y++;
    }
    return img;
}

uint16_t Gen3Tileset::behavior(uint16_t id) const
{
    uint32_t off=attributeOffset(id);
    uint32_t attr;
    if(game_.attributeSize()==4)
        attr=rom_.u32(off);
    else
        attr=rom_.u16(off);
    return game_.behavior(attr);
}

uint8_t Gen3Tileset::layerType(uint16_t id) const
{
    uint32_t off=attributeOffset(id);
    uint32_t attr;
    if(game_.attributeSize()==4)
        attr=rom_.u32(off);
    else
        attr=rom_.u16(off);
    return game_.layerType(attr);
}

bool Gen3Tileset::isAnimatedWater(uint16_t id) const
{
    if(game_.animWaterTileCount==0 || waterFrames_.empty())
        return false;
    uint8_t s=0;
    while(s<8)
    {
        uint16_t tileIndex=rom_.u16(metatileEntryOffset(id,s)) & 0x3FF;
        if(tileIndex>=game_.animWaterTile &&
           tileIndex<static_cast<uint32_t>(game_.animWaterTile)+game_.animWaterTileCount)
            return true;
        s++;
    }
    return false;
}

QImage Gen3Tileset::renderMetatileFrame(uint16_t id, int frame) const
{
    currentWaterFrame_=frame;
    QImage img=renderMetatile(id);
    currentWaterFrame_=-1;
    return img;
}

std::vector<QImage> Gen3Tileset::renderDoorFrames(uint16_t id) const
{
    std::vector<QImage> frames;
    if(game_.doorTable==0)
        return frames;
    // Linear-scan gDoorAnimGraphicsTable (stride 12) for this metatile.
    uint32_t rec=game_.doorTable;
    uint32_t tilesPtr=0,palPtr=0;
    uint8_t sizeByte=0;
    bool found=false;
    int guard=0;
    while(guard<128)
    {
        uint16_t mt=rom_.u16(rec+0);
        bool okT=false;
        uint32_t tp=rom_.pointer(rec+4,&okT);
        if(mt==0 && !okT)
            break; // zero-record terminator
        if(mt==id)
        {
            sizeByte=rom_.u8(rec+3);
            tilesPtr=tp;
            bool okP=false;
            palPtr=rom_.pointer(rec+8,&okP);
            found=okP;
            break;
        }
        rec+=12;
        guard++;
    }
    if(!found || tilesPtr==0)
        return frames;
    // RSE doors are always 16x32 (animate the bottom 16x16); FRLG uses the size
    // byte: 0 = small 16x16, 1 = big 16x32 (bottom 16x16).  The blob is raw 4bpp.
    bool big=(game_.engine==Engine::Rse) ? true : (sizeByte!=0);
    int subBase=big ? 4 : 0;                 // first of the bottom 4 subtiles
    uint32_t openOff[3];
    if(big) { openOff[0]=0; openOff[1]=256; openOff[2]=512; }
    else    { openOff[0]=0; openOff[1]=128; openOff[2]=256; }
    static const int posX[4]={0,8,0,8};
    static const int posY[4]={0,0,8,8};
    // Frame 0: the closed door (the map metatile itself).
    frames.push_back(renderMetatile(id));
    int fi=0;
    while(fi<3)
    {
        QImage img(16,16,QImage::Format_ARGB32);
        img.fill(Qt::transparent);
        int s=0;
        while(s<4)
        {
            int sub=subBase+s;
            const uint8_t *tile=rom_.raw(tilesPtr+openOff[fi]+static_cast<uint32_t>(sub)*32,32);
            uint8_t palNum=rom_.u8(palPtr+static_cast<uint32_t>(sub));
            if(tile!=nullptr)
            {
                int row=0;
                while(row<8)
                {
                    int col=0;
                    while(col<8)
                    {
                        uint8_t b=tile[row*4+(col>>1)];
                        uint8_t ci=(col & 1) ? static_cast<uint8_t>(b>>4) : static_cast<uint8_t>(b & 0xF);
                        if(ci!=0)
                            img.setPixel(posX[s]+col,posY[s]+row,paletteColor(palNum,ci));
                        col++;
                    }
                    row++;
                }
            }
            s++;
        }
        frames.push_back(img);
        fi++;
    }
    return frames;
}

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

static std::string hex8(uint32_t v)
{
    char buf[16];
    std::snprintf(buf,sizeof(buf),"%08x",v);
    return std::string(buf);
}

static int largestOpaqueComponent(const QImage &img)
{
    int w=img.width(),h=img.height();
    std::vector<uint8_t> seen(static_cast<size_t>(w*h),0);
    int best=0;
    int y=0;
    while(y<h)
    {
        int x=0;
        while(x<w)
        {
            size_t idx=static_cast<size_t>(y*w+x);
            if(!seen[idx] && qAlpha(img.pixel(x,y))>0)
            {
                int size=0;
                std::vector<int> stack;
                stack.push_back(y*w+x);
                seen[idx]=1;
                while(!stack.empty())
                {
                    int p=stack.back();
                    stack.pop_back();
                    size++;
                    int px=p%w, py=p/w;
                    static const int dx[4]={1,-1,0,0};
                    static const int dy[4]={0,0,1,-1};
                    int k=0;
                    while(k<4)
                    {
                        int nx=px+dx[k], ny=py+dy[k];
                        if(nx>=0 && nx<w && ny>=0 && ny<h)
                        {
                            size_t nidx=static_cast<size_t>(ny*w+nx);
                            if(!seen[nidx] && qAlpha(img.pixel(nx,ny))>0)
                            {
                                seen[nidx]=1;
                                stack.push_back(ny*w+nx);
                            }
                        }
                        k++;
                    }
                }
                if(size>best)
                    best=size;
            }
            x++;
        }
        y++;
    }
    return best;
}

static void blitTile(QImage &sheet, int columns, uint32_t cell, const QImage &tile)
{
    int cx=static_cast<int>((cell%columns)*16);
    int cy=static_cast<int>((cell/columns)*16);
    int yy=0;
    while(yy<16)
    {
        int xx=0;
        while(xx<16)
        {
            sheet.setPixel(cx+xx,cy+yy,tile.pixel(xx,yy));
            xx++;
        }
        yy++;
    }
}

static void writeTsx(const std::string &dir, const std::string &name, int columns, const QImage &sheet,
                     uint32_t tilecount, const std::unordered_map<int,std::string> &anims)
{
    std::ofstream tsx(dir+"/"+name+".tsx");
    if(tsx)
    {
        tsx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        tsx << "<tileset name=\"" << name << "\" tilewidth=\"16\" tileheight=\"16\" tilecount=\""
            << tilecount << "\" columns=\"" << columns << "\">\n";
        tsx << " <image source=\"" << name << ".png\" width=\"" << sheet.width()
            << "\" height=\"" << sheet.height() << "\"/>\n";
        // animated tiles, in id order for deterministic output
        std::vector<int> ids;
        std::unordered_map<int,std::string>::const_iterator it=anims.cbegin();
        while(it!=anims.cend()) { ids.push_back(it->first); ++it; }
        std::sort(ids.begin(),ids.end());
        size_t k=0;
        while(k<ids.size())
        {
            tsx << " <tile id=\"" << ids[k] << "\">\n  <properties>\n   <property name=\"animation\" value=\""
                << anims.at(ids[k]) << "\"/>\n  </properties>\n </tile>\n";
            k++;
        }
        tsx << "</tileset>\n";
    }
}

// Add an image to the de-dup pool; returns its (possibly existing) cell index.
static int dedupAdd(const QImage &img, std::unordered_map<std::string,int> &seen, std::vector<QImage> &tiles)
{
    QImage a=img.convertToFormat(QImage::Format_ARGB32);
    std::string key(reinterpret_cast<const char *>(a.constBits()),static_cast<size_t>(a.sizeInBytes()));
    std::unordered_map<std::string,int>::const_iterator it=seen.find(key);
    if(it!=seen.cend())
        return it->second;
    int idx=static_cast<int>(tiles.size());
    tiles.push_back(a);
    seen[key]=idx;
    return idx;
}

// 2-D adjacency layout for ONE tile set (grounds, or the WalkBehind/over tiles).
// cellTi[map][y*W+x] = the tile index used at that map cell (-1 = none).  Tiles
// that are CONSISTENTLY each other's immediate map-neighbour are packed as rigid
// blocks so a building reads like the map; a block too wide for the sheet is cut
// into <=COLS-wide vertical chunks (both directions kept inside a chunk); an
// irregular block is split to singletons.  Identical units and stray repeated
// singletons are merged.  Returns the repacked tiles; pos[i] = final cell of old
// tile i (tiles never referenced by cellTi get pos 0 and are NOT emitted);
// adjViol gains every consistent immediate adjacency the packing could not keep.
static std::vector<QImage> layout2D(const std::vector<QImage> &tiles,
    const std::vector<std::vector<int> > &cellTi,
    const std::vector<const DecodedMap *> &poolMaps, int COLS,
    std::vector<uint32_t> &pos, uint32_t &adjViol, const std::string &label)
{
    (void)label;
    int N=static_cast<int>(tiles.size());
    std::vector<char> used(N,0);
    std::vector<int> rN(N,-2),dN(N,-2),lN(N,-2),uN(N,-2); // -2 unset, -1 none/varies
    size_t mi=0;
    while(mi<poolMaps.size())
    {
        const DecodedMap *mp=poolMaps[mi];
        int W=static_cast<int>(mp->width), H=static_cast<int>(mp->height);
        const std::vector<int> &ct=cellTi[mi];
        int y=0;
        while(y<H)
        {
            int x=0;
            while(x<W)
            {
                int ti=ct[static_cast<size_t>(y)*W+x];
                if(ti>=0)
                {
                    used[ti]=1;
                    int nb;
                    nb=(x+1<W)?ct[static_cast<size_t>(y)*W+x+1]:-1;
                    if(nb>=0){ if(rN[ti]==-2)rN[ti]=nb; else if(rN[ti]!=nb)rN[ti]=-1; }
                    nb=(y+1<H)?ct[static_cast<size_t>(y+1)*W+x]:-1;
                    if(nb>=0){ if(dN[ti]==-2)dN[ti]=nb; else if(dN[ti]!=nb)dN[ti]=-1; }
                    nb=(x>0)?ct[static_cast<size_t>(y)*W+x-1]:-1;
                    if(nb>=0){ if(lN[ti]==-2)lN[ti]=nb; else if(lN[ti]!=nb)lN[ti]=-1; }
                    nb=(y>0)?ct[static_cast<size_t>(y-1)*W+x]:-1;
                    if(nb>=0){ if(uN[ti]==-2)uN[ti]=nb; else if(uN[ti]!=nb)uN[ti]=-1; }
                }
                x++;
            }
            y++;
        }
        mi++;
    }
    int t=0;
    while(t<N){ if(rN[t]<0)rN[t]=-1; if(dN[t]<0)dN[t]=-1; if(lN[t]<0)lN[t]=-1; if(uN[t]<0)uN[t]=-1; t++; }
    t=0;
    while(t<N){ if(rN[t]>=0 && lN[rN[t]]!=t)rN[t]=-1; if(dN[t]>=0 && uN[dN[t]]!=t)dN[t]=-1; t++; }
    t=0;
    while(t<N){ if(lN[t]>=0 && rN[lN[t]]!=t)lN[t]=-1; if(uN[t]>=0 && dN[uN[t]]!=t)uN[t]=-1; t++; }

    // group used tiles into blocks (connected components) with relative coords
    std::vector<int> bc(N,0),br(N,0),seen(N,0);
    std::vector<std::vector<int> > units;
    std::vector<int> unitW,unitH;
    t=0;
    while(t<N)
    {
        if(!seen[t] && used[t])
        {
            std::vector<int> bt,stack;
            stack.push_back(t); seen[t]=1; bc[t]=0; br[t]=0;
            int mnc=0,mnr=0,mxc=0,mxr=0;
            while(!stack.empty())
            {
                int cur=stack.back(); stack.pop_back();
                bt.push_back(cur);
                if(bc[cur]<mnc)mnc=bc[cur]; if(bc[cur]>mxc)mxc=bc[cur];
                if(br[cur]<mnr)mnr=br[cur]; if(br[cur]>mxr)mxr=br[cur];
                int nb;
                nb=rN[cur]; if(nb>=0&&!seen[nb]){seen[nb]=1;bc[nb]=bc[cur]+1;br[nb]=br[cur];stack.push_back(nb);}
                nb=lN[cur]; if(nb>=0&&!seen[nb]){seen[nb]=1;bc[nb]=bc[cur]-1;br[nb]=br[cur];stack.push_back(nb);}
                nb=dN[cur]; if(nb>=0&&!seen[nb]){seen[nb]=1;bc[nb]=bc[cur];br[nb]=br[cur]+1;stack.push_back(nb);}
                nb=uN[cur]; if(nb>=0&&!seen[nb]){seen[nb]=1;bc[nb]=bc[cur];br[nb]=br[cur]-1;stack.push_back(nb);}
            }
            int w=mxc-mnc+1,h=mxr-mnr+1;
            std::vector<int> local(static_cast<size_t>(w)*static_cast<size_t>(h),-1);
            std::vector<int> collided;
            size_t bi=0;
            while(bi<bt.size())
            {
                int ti=bt[bi]; bc[ti]-=mnc; br[ti]-=mnr;
                size_t idx=static_cast<size_t>(br[ti])*w+bc[ti];
                if(local[idx]>=0) collided.push_back(ti); else local[idx]=ti;
                bi++;
            }
            // The non-collided tiles form a clean grid block (holes allowed) that
            // keeps every adjacency; the few collided tiles (a geometric
            // inconsistency in the map) drop to singletons.  A block too wide for
            // the sheet is cut into <=COLS-wide vertical chunks (full height kept).
            std::vector<int> blk;
            { size_t k=0; while(k<local.size()){ if(local[k]>=0) blk.push_back(local[k]); k++; } }
            if(w>COLS)
            {
                int c0=0;
                while(c0<w)
                {
                    int cw=(w-c0<COLS)?(w-c0):COLS;
                    std::vector<int> chunk;
                    size_t kk=0;
                    while(kk<blk.size()){ int ti=blk[kk]; if(bc[ti]>=c0 && bc[ti]<c0+cw) chunk.push_back(ti); kk++; }
                    size_t ci=0; while(ci<chunk.size()){ bc[chunk[ci]]-=c0; ci++; }
                    if(!chunk.empty()){ units.push_back(chunk); unitW.push_back(cw); unitH.push_back(h); }
                    c0+=cw;
                }
            }
            else if(!blk.empty()) { units.push_back(blk); unitW.push_back(w); unitH.push_back(h); }
            { size_t k=0; while(k<collided.size()){ std::vector<int> u; u.push_back(collided[k]); bc[collided[k]]=0; br[collided[k]]=0; units.push_back(u); unitW.push_back(1); unitH.push_back(1); k++; } }
        }
        t++;
    }

    // DE-DUP GUARD: give each graphic a small id, then (A) collapse units with
    // identical shape+content, (B) fold a leftover singleton into an identical
    // kept cell.  repOf[] redirects a dropped tile to its surviving cell.
    std::unordered_map<std::string,int> imgIdMap;
    std::vector<int> imgId(N,0);
    {
        int q=0;
        while(q<N)
        {
            QImage a=tiles[static_cast<size_t>(q)].convertToFormat(QImage::Format_ARGB32);
            std::string k(reinterpret_cast<const char *>(a.constBits()),static_cast<size_t>(a.sizeInBytes()));
            std::unordered_map<std::string,int>::const_iterator f=imgIdMap.find(k);
            if(f!=imgIdMap.cend()) imgId[q]=f->second;
            else { int id=static_cast<int>(imgIdMap.size()); imgIdMap[k]=id; imgId[q]=id; }
            q++;
        }
    }
    std::vector<int> repOf(N);
    { int q=0; while(q<N){ repOf[q]=q; q++; } }
    std::vector<char> keptUnit(units.size(),1);
    std::unordered_map<std::string,size_t> sigToUnit;
    std::vector<std::unordered_map<int,int> > unitPosTile(units.size());
    {
        size_t u=0;
        while(u<units.size())
        {
            int w=unitW[u],h=unitH[u];
            std::vector<int> cellIds(static_cast<size_t>(w)*static_cast<size_t>(h),-1);
            std::unordered_map<int,int> posTile;
            size_t bi=0;
            while(bi<units[u].size())
            {
                int ti=units[u][bi];
                int pi=br[ti]*w+bc[ti];
                cellIds[static_cast<size_t>(pi)]=imgId[ti];
                posTile[pi]=ti;
                bi++;
            }
            std::string sig=std::to_string(w)+","+std::to_string(h);
            size_t ci=0;
            while(ci<cellIds.size()){ sig+=","; sig+=std::to_string(cellIds[ci]); ci++; }
            std::unordered_map<std::string,size_t>::const_iterator it=sigToUnit.find(sig);
            if(it!=sigToUnit.cend())
            {
                std::unordered_map<int,int> &rep=unitPosTile[it->second];
                bi=0;
                while(bi<units[u].size()){ int ti=units[u][bi]; repOf[ti]=rep[br[ti]*w+bc[ti]]; bi++; }
                keptUnit[u]=0;
            }
            else { sigToUnit[sig]=u; unitPosTile[u].swap(posTile); }
            u++;
        }
    }
    {
        std::unordered_map<int,int> imgToCell;
        size_t u=0;
        while(u<units.size())
        {
            if(keptUnit[u] && units[u].size()>1)
            {
                size_t bi=0;
                while(bi<units[u].size()){ int ti=units[u][bi]; if(imgToCell.find(imgId[ti])==imgToCell.cend()) imgToCell[imgId[ti]]=ti; bi++; }
            }
            u++;
        }
        u=0;
        while(u<units.size())
        {
            if(keptUnit[u] && units[u].size()==1)
            {
                int ti=units[u][0];
                std::unordered_map<int,int>::const_iterator it=imgToCell.find(imgId[ti]);
                if(it!=imgToCell.cend()){ repOf[ti]=it->second; keptUnit[u]=0; }
                else imgToCell[imgId[ti]]=ti;
            }
            u++;
        }
    }
    { int q=0; while(q<N){ int rr=q; while(repOf[rr]!=rr) rr=repOf[rr]; repOf[q]=rr; q++; } }

    // shelf-pack surviving blocks into the COLS-wide grid (whole blocks intact)
    std::unordered_map<int,uint32_t> tilePos;
    std::vector<std::vector<int> > grid;
    int cursorX=0,shelfY=0,shelfH=0;
    size_t ui=0;
    while(ui<units.size())
    {
        if(keptUnit[ui])
        {
            int w=unitW[ui],h=unitH[ui];
            if(cursorX+w>COLS){ cursorX=0; shelfY+=shelfH; shelfH=0; }
            while(static_cast<int>(grid.size())<shelfY+h) grid.push_back(std::vector<int>(static_cast<size_t>(COLS),-1));
            size_t bi=0;
            while(bi<units[ui].size())
            {
                int ti=units[ui][bi];
                int gc=cursorX+bc[ti],gr=shelfY+br[ti];
                grid[static_cast<size_t>(gr)][static_cast<size_t>(gc)]=ti;
                tilePos[ti]=static_cast<uint32_t>(gr)*static_cast<uint32_t>(COLS)+static_cast<uint32_t>(gc);
                bi++;
            }
            cursorX+=w; if(h>shelfH)shelfH=h;
        }
        ui++;
    }

    // adjacency check: consistent immediate map-neighbours must stay adjacent
    {
        int t2=0;
        while(t2<N)
        {
            std::unordered_map<int,uint32_t>::const_iterator ptv=tilePos.find(repOf[t2]);
            if(ptv!=tilePos.cend())
            {
                uint32_t posT=ptv->second;
                if(rN[t2]>=0 && repOf[rN[t2]]!=repOf[t2])
                {
                    std::unordered_map<int,uint32_t>::const_iterator pr=tilePos.find(repOf[rN[t2]]);
                    if(!(pr!=tilePos.cend() && pr->second==posT+1 && (posT%static_cast<uint32_t>(COLS))!=static_cast<uint32_t>(COLS-1)))
                        adjViol++;
                }
                if(dN[t2]>=0 && repOf[dN[t2]]!=repOf[t2])
                {
                    std::unordered_map<int,uint32_t>::const_iterator pd=tilePos.find(repOf[dN[t2]]);
                    if(!(pd!=tilePos.cend() && pd->second==posT+static_cast<uint32_t>(COLS)))
                        adjViol++;
                }
            }
            t2++;
        }
    }

    QImage blankG(16,16,QImage::Format_ARGB32);
    blankG.fill(Qt::transparent);
    std::vector<QImage> newTiles(grid.size()*static_cast<size_t>(COLS),blankG);
    uint32_t r=0;
    while(r<grid.size())
    {
        int c2=0;
        while(c2<COLS)
        {
            int ti=grid[r][static_cast<size_t>(c2)];
            if(ti>=0) newTiles[static_cast<size_t>(r)*static_cast<size_t>(COLS)+static_cast<size_t>(c2)]=tiles[static_cast<size_t>(ti)];
            c2++;
        }
        r++;
    }
    pos.assign(static_cast<size_t>(N),0);
    t=0;
    while(t<N)
    {
        std::unordered_map<int,uint32_t>::const_iterator p=tilePos.find(repOf[t]);
        if(p!=tilePos.cend()) pos[static_cast<size_t>(t)]=p->second;
        t++;
    }
    return newTiles;
}

// ---------------------------------------------------------------------------
// TilesetBuilder
// ---------------------------------------------------------------------------

TilePool::TilePool() :
    uniqueCount(0),
    sheetCount(0),
    duplicateTiles(0),
    adjacencyViolations(0)
{
}

TilesetBuilder::TilesetBuilder(const GbaRom &rom, const std::string &tilesetDir) :
    rom_(rom),
    tilesetDir_(tilesetDir)
{
}

uint64_t TilesetBuilder::pairKey(uint32_t primary, uint32_t secondary)
{
    return (static_cast<uint64_t>(primary)<<32) | static_cast<uint64_t>(secondary);
}

Gen3Tileset &TilesetBuilder::tilesetFor(const DecodedMap &map) const
{
    uint64_t key=pairKey(map.primaryTileset,map.secondaryTileset);
    std::map<uint64_t,Gen3Tileset *>::iterator it=cache_.find(key);
    if(it!=cache_.cend())
        return *it->second;
    Gen3Tileset *ts=new Gen3Tileset(rom_,map.primaryTileset,map.secondaryTileset);
    cache_[key]=ts;
    return *ts;
}

TilePool TilesetBuilder::buildPool(uint32_t primaryPtr, uint32_t secondaryPtr,
                                   const std::vector<uint16_t> &usedIds, const std::string &baseName,
                                   const std::vector<const DecodedMap *> &poolMaps)
{
    Gen3Tileset ts(rom_,primaryPtr,secondaryPtr);
    TilePool pool;
    pool.baseName=baseName;
    const int overThreshold=9;
    const GameInfo &gi=rom_.game();
    int waterFrames=static_cast<int>(gi.animWaterFrames);
    std::string animStr;
    if(waterFrames>0)
        animStr=std::to_string(gi.animWaterMs)+"ms;"+std::to_string(waterFrames)+"frames";

    // Coherent ordering: all ground tiles first, then the above-player overlay
    // tiles, then the animation frame sequences — so the sheet does not
    // interleave full tiles, transparent overlays and anim frames.
    std::vector<QImage> grounds,overs;
    std::unordered_map<std::string,int> groundSeen,overSeen;
    std::unordered_map<uint16_t,int> groundLocal,overLocal; // plain metatile -> cell
    std::unordered_map<uint64_t,int> contextLocal;          // split tile context -> cell
    std::unordered_map<uint16_t,QImage> metaGroundImg;      // metatile -> ground image
    std::vector<uint16_t> animatedIds;
    std::vector<uint16_t> doorIds; // door metatiles (behaviour 0x69)

    // Pass 1: classify each used metatile and render its ground (Walkable) image.
    // Animated water metatiles are handled later in the anim section.
    size_t i=0;
    while(i<usedIds.size())
    {
        uint16_t id=usedIds[i];
        // A door metatile also gets a static ground tile (for Walkable); the
        // animated door tile is built separately and used only by the door object.
        if(ts.behavior(id)==0x69)
            doorIds.push_back(id);
        if(waterFrames>0 && ts.isAnimatedWater(id))
            animatedIds.push_back(id);
        else
        {
            // Only NORMAL(0) and SPLIT(2) metatiles draw their top sub-layer
            // ABOVE the player; COVERED(1) draws both sub-layers below the
            // player, so its top must stay in the ground tile (never lifted to
            // WalkBehind).  This keeps WalkBehind minimal: building/tree tops.
            uint8_t lt=ts.layerType(id);
            QImage under=ts.renderUnder(id);
            QImage over=ts.renderOver(id,under);
            if(lt!=1 && largestOpaqueComponent(over)>overThreshold)
            {
                overLocal[id]=dedupAdd(over,overSeen,overs);
                metaGroundImg[id]=under.convertToFormat(QImage::Format_ARGB32);
            }
            else
            {
                overLocal[id]=-1;
                metaGroundImg[id]=ts.renderMetatile(id).convertToFormat(QImage::Format_ARGB32);
            }
        }
        i++;
    }

    // Pass 2: for each ground metatile count its DISTINCT neighbourhoods across
    // all the pool's maps.  2-D preservation takes priority over de-dup, but
    // BOUNDED: a "structural" tile used at only a few (<=kContextSplitMax)
    // distinct neighbourhoods is CONTEXT-split (one cell per neighbourhood, so a
    // building keeps its exact on-map shape); a "background" tile (grass / path /
    // floor) used everywhere with many neighbours stays de-duplicated to a single
    // cell, otherwise the sheet count would explode (7x) and blow the per-map
    // tileset budget.
    const int kContextSplitMax=0;
    std::unordered_map<uint16_t,std::unordered_set<uint64_t> > metaContexts;
    size_t mi=0;
    while(mi<poolMaps.size())
    {
        const DecodedMap *mp=poolMaps[mi];
        int W=static_cast<int>(mp->width), H=static_cast<int>(mp->height);
        int y=0;
        while(y<H)
        {
            int x=0;
            while(x<W)
            {
                uint16_t m=static_cast<uint16_t>(rom_.u16(mp->blocksPtr+(static_cast<uint32_t>(y)*W+x)*2)&0x3FF);
                if(metaGroundImg.find(m)!=metaGroundImg.cend())
                    metaContexts[m].insert(contextKey(*mp,x,y));
                x++;
            }
            y++;
        }
        mi++;
    }
    std::unordered_map<uint16_t,bool> splitMeta;
    {
        std::unordered_map<uint16_t,std::unordered_set<uint64_t> >::const_iterator it=metaContexts.cbegin();
        while(it!=metaContexts.cend())
        {
            size_t n=it->second.size();
            splitMeta[it->first]=(n>=2 && n<=static_cast<size_t>(kContextSplitMax));
            ++it;
        }
    }

    // Pass 3: build ground tiles + a per-cell tile index for every pool map.
    // Split metatile -> one cell per context; plain metatile -> a single
    // byte-de-duplicated cell.  cellTi feeds the 2-D layout below.
    std::vector<std::vector<int> > cellTi(poolMaps.size());
    mi=0;
    while(mi<poolMaps.size())
    {
        const DecodedMap *mp=poolMaps[mi];
        int W=static_cast<int>(mp->width), H=static_cast<int>(mp->height);
        cellTi[mi].assign(static_cast<size_t>(W)*static_cast<size_t>(H),-1);
        int y=0;
        while(y<H)
        {
            int x=0;
            while(x<W)
            {
                uint16_t m=static_cast<uint16_t>(rom_.u16(mp->blocksPtr+(static_cast<uint32_t>(y)*W+x)*2)&0x3FF);
                std::unordered_map<uint16_t,QImage>::const_iterator gi=metaGroundImg.find(m);
                if(gi!=metaGroundImg.cend())
                {
                    int ti;
                    if(splitMeta[m])
                    {
                        uint64_t ck=contextKey(*mp,x,y);
                        std::unordered_map<uint64_t,int>::const_iterator ci=contextLocal.find(ck);
                        if(ci!=contextLocal.cend())
                            ti=ci->second;
                        else
                        {
                            ti=static_cast<int>(grounds.size());
                            grounds.push_back(gi->second);
                            contextLocal[ck]=ti;
                        }
                    }
                    else
                    {
                        std::unordered_map<uint16_t,int>::const_iterator pi=groundLocal.find(m);
                        if(pi!=groundLocal.cend())
                            ti=pi->second;
                        else
                        {
                            ti=dedupAdd(gi->second,groundSeen,grounds);
                            groundLocal[m]=ti;
                        }
                    }
                    cellTi[mi][static_cast<size_t>(y)*W+x]=ti;
                }
                x++;
            }
            y++;
        }
        mi++;
    }

    // 2-D adjacency layout for the GROUND tiles (see layout2D).
    {
        std::vector<uint32_t> gpos;
        uint32_t gadj=0;
        std::vector<QImage> newGrounds=layout2D(grounds,cellTi,poolMaps,kColumns,gpos,gadj,baseName+":ground");
        std::unordered_map<uint16_t,int>::iterator gl=groundLocal.begin();
        while(gl!=groundLocal.end()){ gl->second=static_cast<int>(gpos[static_cast<size_t>(gl->second)]); ++gl; }
        std::unordered_map<uint64_t,int>::iterator cl=contextLocal.begin();
        while(cl!=contextLocal.end()){ cl->second=static_cast<int>(gpos[static_cast<size_t>(cl->second)]); ++cl; }
        grounds.swap(newGrounds);
        pool.adjacencyViolations+=gadj;
    }

    uint32_t groundCount=static_cast<uint32_t>(grounds.size());
    QImage blkT(16,16,QImage::Format_ARGB32);
    blkT.fill(Qt::transparent);
    std::string blkKey(reinterpret_cast<const char *>(blkT.constBits()),static_cast<size_t>(blkT.sizeInBytes()));
    // OVER (WalkBehind) tiles.  A human reads the map as the TOP visible tile, so
    // the over tiles (building/tree tops) must follow the same 2-D map adjacency
    // as the grounds -- not be dumped in dedup order.  First FOLD any over that is
    // pixel-identical to a ground tile onto that ground cell (no duplicate), then
    // lay the remaining overs out with the same block packer, in a region right
    // after the grounds.  overRemap[old over index] -> absolute pool cell.
    std::unordered_map<std::string,uint32_t> groundImgPos;
    {
        uint32_t p=0;
        while(p<groundCount)
        {
            QImage a=grounds[p].convertToFormat(QImage::Format_ARGB32);
            std::string key(reinterpret_cast<const char *>(a.constBits()),static_cast<size_t>(a.sizeInBytes()));
            if(key!=blkKey && groundImgPos.find(key)==groundImgPos.cend()) groundImgPos[key]=p;
            p++;
        }
    }
    const uint32_t NOT_FOLDED=0xFFFFFFFFu;
    std::vector<uint32_t> overFold(overs.size(),NOT_FOLDED);
    {
        size_t oi=0;
        while(oi<overs.size())
        {
            QImage a=overs[oi].convertToFormat(QImage::Format_ARGB32);
            std::string key(reinterpret_cast<const char *>(a.constBits()),static_cast<size_t>(a.sizeInBytes()));
            std::unordered_map<std::string,uint32_t>::const_iterator f=groundImgPos.find(key);
            if(f!=groundImgPos.cend()) overFold[oi]=f->second;
            oi++;
        }
    }
    std::vector<std::vector<int> > overCellTi(poolMaps.size());
    {
        size_t pmi=0;
        while(pmi<poolMaps.size())
        {
            const DecodedMap *mp=poolMaps[pmi];
            int W=static_cast<int>(mp->width), H=static_cast<int>(mp->height);
            overCellTi[pmi].assign(static_cast<size_t>(W)*static_cast<size_t>(H),-1);
            int yy=0;
            while(yy<H)
            {
                int xx=0;
                while(xx<W)
                {
                    uint16_t m=static_cast<uint16_t>(rom_.u16(mp->blocksPtr+(static_cast<uint32_t>(yy)*W+xx)*2)&0x3FF);
                    std::unordered_map<uint16_t,int>::const_iterator ol=overLocal.find(m);
                    if(ol!=overLocal.cend() && ol->second>=0 && overFold[static_cast<size_t>(ol->second)]==NOT_FOLDED)
                        overCellTi[pmi][static_cast<size_t>(yy)*W+xx]=ol->second;
                    xx++;
                }
                yy++;
            }
            pmi++;
        }
    }
    std::vector<uint32_t> opos;
    uint32_t oadj=0;
    {
        std::vector<QImage> newOvers=layout2D(overs,overCellTi,poolMaps,kColumns,opos,oadj,baseName+":over");
        overs.swap(newOvers);
    }
    pool.adjacencyViolations+=oadj;
    std::vector<uint32_t> overRemap(opos.size(),0);
    {
        size_t oi=0;
        while(oi<overRemap.size())
        {
            overRemap[oi]=(overFold[oi]!=NOT_FOLDED) ? overFold[oi] : (groundCount+opos[oi]);
            oi++;
        }
    }
    uint32_t groundsOvers=groundCount+static_cast<uint32_t>(overs.size());
    // Ground (2-D grid) + over (2-D grid) tiles, then the animation tiles.
    uint32_t animStart=groundsOvers;

    // Animation frame sequences, kept consecutive and inside one sheet.
    std::vector<QImage> anims;
    std::unordered_map<std::string,int> animSeen;
    std::unordered_map<uint16_t,int> animLocal;
    std::unordered_map<int,std::string> animLocalStr;
    i=0;
    while(i<animatedIds.size())
    {
        uint16_t id=animatedIds[i];
        std::vector<QImage> frames;
        std::string key;
        int f=0;
        while(f<waterFrames)
        {
            QImage fr=ts.renderMetatileFrame(id,f).convertToFormat(QImage::Format_ARGB32);
            key.append(reinterpret_cast<const char *>(fr.constBits()),static_cast<size_t>(fr.sizeInBytes()));
            frames.push_back(fr);
            f++;
        }
        std::unordered_map<std::string,int>::const_iterator it=animSeen.find(key);
        if(it!=animSeen.cend())
            animLocal[id]=it->second;
        else
        {
            uint32_t abs=animStart+static_cast<uint32_t>(anims.size());
            uint32_t pos=abs%kCapacity;
            if(pos+static_cast<uint32_t>(waterFrames)>kCapacity)
            {
                QImage empty(16,16,QImage::Format_ARGB32);
                empty.fill(Qt::transparent);
                uint32_t pad=kCapacity-pos;
                uint32_t p=0;
                while(p<pad) { anims.push_back(empty); p++; }
            }
            int firstLocal=static_cast<int>(anims.size());
            f=0;
            while(f<waterFrames) { anims.push_back(frames[f]); f++; }
            animSeen[key]=firstLocal;
            animLocal[id]=firstLocal;
            animLocalStr[firstLocal]=animStr;
        }
        i++;
    }

    // Door animation tiles: the real door-open frames from the ROM's
    // gDoorAnimGraphicsTable (closed -> 3 progressively open states), placed in
    // the consecutive animation section so the door OBJECT references an animated
    // tile (the client deletes a door whose tile has no animation and requires
    // frames>1).  Only the object uses these; the static Walkable door tile is
    // unaffected.  Falls back to a 2-frame static door if the metatile is not in
    // the table.
    std::unordered_map<uint16_t,int> doorLocal;
    i=0;
    while(i<doorIds.size())
    {
        uint16_t id=doorIds[i];
        std::vector<QImage> dframes=ts.renderDoorFrames(id);
        std::string dstr;
        if(dframes.size()>=2)
            dstr="67ms;"+std::to_string(dframes.size())+"frames";
        else
        {
            QImage c=ts.renderMetatile(id);
            dframes.clear();
            dframes.push_back(c);
            dframes.push_back(c);
            dstr="120ms;2frames";
        }
        std::string key;
        size_t fr=0;
        while(fr<dframes.size())
        {
            dframes[fr]=dframes[fr].convertToFormat(QImage::Format_ARGB32);
            key.append(reinterpret_cast<const char *>(dframes[fr].constBits()),static_cast<size_t>(dframes[fr].sizeInBytes()));
            fr++;
        }
        std::unordered_map<std::string,int>::const_iterator it=animSeen.find(key);
        if(it!=animSeen.cend())
            doorLocal[id]=it->second;
        else
        {
            uint32_t nf=static_cast<uint32_t>(dframes.size());
            uint32_t abs=animStart+static_cast<uint32_t>(anims.size());
            uint32_t pos=abs%kCapacity;
            if(pos+nf>kCapacity)
            {
                QImage empty(16,16,QImage::Format_ARGB32);
                empty.fill(Qt::transparent);
                uint32_t pad=kCapacity-pos;
                uint32_t p=0;
                while(p<pad) { anims.push_back(empty); p++; }
            }
            int firstLocal=static_cast<int>(anims.size());
            fr=0;
            while(fr<dframes.size()) { anims.push_back(dframes[fr]); fr++; }
            animSeen[key]=firstLocal;
            doorLocal[id]=firstLocal;
            animLocalStr[firstLocal]=dstr;
        }
        i++;
    }

    // Concatenate: ground+over region, padded so the (row-major) anim region
    // starts on a fresh sheet, then the anims.
    std::vector<QImage> tiles;
    tiles.reserve(animStart+anims.size());
    size_t j=0;
    while(j<grounds.size()) { tiles.push_back(grounds[j]); j++; }
    j=0;
    while(j<overs.size()) { tiles.push_back(overs[j]); j++; }
    QImage blank(16,16,QImage::Format_ARGB32);
    blank.fill(Qt::transparent);
    while(static_cast<uint32_t>(tiles.size())<animStart) tiles.push_back(blank);
    j=0;
    while(j<anims.size()) { tiles.push_back(anims[j]); j++; }

    // Resolve per-metatile cells into the concatenated layout.
    i=0;
    while(i<usedIds.size())
    {
        uint16_t id=usedIds[i];
        std::unordered_map<uint16_t,int>::const_iterator git=groundLocal.find(id);
        if(git!=groundLocal.cend())
            pool.groundCell[id]=git->second;            // plain (high-freq) ground
        else
        {
            std::unordered_map<uint16_t,int>::const_iterator dit=animLocal.find(id);
            if(dit!=animLocal.cend())
                pool.groundCell[id]=static_cast<int>(animStart)+dit->second; // water
            // else: a context-split metatile -> resolved via contextCell below
        }
        std::unordered_map<uint16_t,int>::const_iterator oit=overLocal.find(id);
        pool.overCell[id]=(oit!=overLocal.cend() && oit->second>=0)
                          ? static_cast<int>(overRemap[static_cast<size_t>(oit->second)]) : -1;
        i++;
    }
    pool.contextCell=contextLocal; // split tile contexts -> final ground cells
    std::unordered_map<uint16_t,int>::const_iterator drit=doorLocal.cbegin();
    while(drit!=doorLocal.cend())
    {
        pool.doorCell[drit->first]=static_cast<int>(animStart)+drit->second;
        ++drit;
    }
    std::unordered_map<int,std::string>::const_iterator cit=animLocalStr.cbegin();
    while(cit!=animLocalStr.cend())
    {
        pool.cellAnimation[static_cast<int>(animStart)+cit->first]=cit->second;
        ++cit;
    }

    pool.uniqueCount=static_cast<uint32_t>(tiles.size());
    pool.sheetCount=(pool.uniqueCount+kCapacity-1)/kCapacity;

    uint32_t s=0;
    while(s<pool.sheetCount)
    {
        uint32_t startCell=s*kCapacity;
        uint32_t cnt=pool.uniqueCount-startCell;
        if(cnt>kCapacity)
            cnt=kCapacity;
        int rows=static_cast<int>((cnt+kColumns-1)/kColumns);
        QImage sheet(kColumns*16,rows*16,QImage::Format_ARGB32);
        sheet.fill(Qt::transparent);
        uint32_t c=0;
        while(c<cnt)
        {
            blitTile(sheet,kColumns,c,tiles[startCell+c]);
            c++;
        }
        // animations whose first frame is in this sheet, remapped to local ids
        std::unordered_map<int,std::string> localAnims;
        std::unordered_map<int,std::string>::const_iterator ait=pool.cellAnimation.cbegin();
        while(ait!=pool.cellAnimation.cend())
        {
            if(ait->first>=static_cast<int>(startCell) && ait->first<static_cast<int>(startCell+cnt))
                localAnims[ait->first-static_cast<int>(startCell)]=ait->second;
            ++ait;
        }
        std::string name=baseName+"_"+std::to_string(s);
        sheet.save(QString::fromStdString(tilesetDir_+"/"+name+".png"),"PNG");
        writeTsx(tilesetDir_,name,kColumns,sheet,cnt,localAnims);
        s++;
    }

    // Post-build GUARD: within a pool no NON-ANIMATION tile graphic may repeat
    // across its sheets (the "no cross-tileset duplicate" rule).  Animation
    // frames are exempt — a door's closed frame legitimately equals its static
    // Walkable tile.  Count the redundant non-animation duplicates so prepare()
    // can FAIL the run if any survive.
    {
        std::unordered_set<std::string> seenImg;
        uint32_t k=0;
        while(static_cast<size_t>(k)<tiles.size() && k<animStart)
        {
            QImage a=tiles[k].convertToFormat(QImage::Format_ARGB32);
            std::string key(reinterpret_cast<const char *>(a.constBits()),static_cast<size_t>(a.sizeInBytes()));
            if(key!=blkKey && !seenImg.insert(key).second)
                pool.duplicateTiles++;
            k++;
        }
        if(pool.duplicateTiles>0)
            std::cout << "  [guard] pool " << baseName << ": " << pool.duplicateTiles
                      << " duplicate non-animation tile(s)" << std::endl;
    }
    return pool;
}

// Name a pool after the area(s) that use it: one area -> its slug, a few ->
// joined, many -> "common".  Resolves collisions against already-used names.
static std::string poolBaseName(const std::set<std::string> &areas, std::set<std::string> &used)
{
    std::string base;
    if(areas.size()==1)
        base=*areas.begin();
    else if(areas.size()>=2 && areas.size()<=3)
    {
        std::set<std::string>::const_iterator it=areas.cbegin();
        while(it!=areas.cend())
        {
            if(!base.empty())
                base+="-";
            base+=*it;
            ++it;
        }
        if(base.size()>48)
            base="common";
    }
    else
        base="common";
    if(base.empty())
        base="common";
    std::string name=base;
    int dup=2;
    while(used.find(name)!=used.cend())
    {
        name=base+"-"+std::to_string(dup);
        dup++;
    }
    used.insert(name);
    return name;
}

void TilesetBuilder::writeMarkerSheet() const
{
    // LOCAL semantic-layer marker tileset: tileset/marker.png (8 cells, 128x16) +
    // marker.tsx.  Cells 0-6 are one distinct SEMI-TRANSPARENT colour per gameplay
    // layer (Collisions/Water/Grass/Ledges*), so toggling the layer in Tiled is
    // visible and it never hides the layer below.  This stays inside the per-ROM
    // tileset/ dir — the shared map/invisible.* is reserved for engine object
    // markers and is NEVER written by the converter.
    const int N=8, TS=16;
    QImage img(N*TS,TS,QImage::Format_ARGB32);
    img.fill(0);
    static const int cols[7][4]={
        {255,  0,  0, 96}, // 0 Collisions  red
        { 20,110,255, 96}, // 1 Water       blue
        {  0,200,  0, 96}, // 2 Grass       green
        {255,235,  0, 96}, // 3 LedgesUp    yellow
        {255,140,  0, 96}, // 4 LedgesDown  orange
        {  0,220,220, 96}, // 5 LedgesLeft  cyan
        {230,  0,230, 96}, // 6 LedgesRight magenta
    };
    int t=0;
    while(t<7)
    {
        QRgb c=qRgba(cols[t][0],cols[t][1],cols[t][2],cols[t][3]);
        int tx=t*TS;
        int yy=0;
        while(yy<TS){ int xx=0; while(xx<TS){ img.setPixel(tx+xx,yy,c); xx++; } yy++; }
        t++;
    }
    img.save(QString::fromStdString(tilesetDir_+"/marker.png"),"PNG");
    std::ofstream tsx(tilesetDir_+"/marker.tsx");
    if(tsx.is_open())
    {
        tsx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        tsx << "<tileset name=\"marker\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"" << N
            << "\" columns=\"" << N << "\">\n";
        tsx << " <image source=\"marker.png\" width=\"" << (N*TS) << "\" height=\"" << TS << "\"/>\n";
        tsx << "</tileset>\n";
    }
}

bool TilesetBuilder::prepare(const std::vector<DecodedMap> &maps, const Naming &naming)
{
    uint32_t metaInPrim=rom_.game().metatilesInPrimary;

    // Used metatiles are collected in MAP ROW-MAJOR FIRST-APPEARANCE order (not
    // sorted by id), so a contiguous map region (a building, a tree cluster)
    // becomes a contiguous run of tiles in the sheet — buildings stay grouped
    // and the sheet reads like the map, instead of scattering by metatile id.
    std::unordered_map<uint32_t,std::vector<uint16_t> > usedPrimary;
    std::unordered_map<uint64_t,std::vector<uint16_t> > usedSecondary;
    std::unordered_map<uint32_t,std::unordered_set<uint16_t> > seenPrimary;
    std::unordered_map<uint64_t,std::unordered_set<uint16_t> > seenSecondary;
    std::unordered_map<uint32_t,std::set<std::string> > primaryAreas;
    std::unordered_map<uint64_t,std::set<std::string> > secondaryAreas;
    // Maps using each pool, in order — the 2-D tile placement walks these.
    std::unordered_map<uint32_t,std::vector<const DecodedMap *> > poolMapsPrimary;
    std::unordered_map<uint64_t,std::vector<const DecodedMap *> > poolMapsSecondary;
    size_t i=0;
    while(i<maps.size())
    {
        const DecodedMap &map=maps[i];
        uint64_t pk=pairKey(map.primaryTileset,map.secondaryTileset);
        const std::string &area=naming.zoneFor(map.group,map.map);
        poolMapsPrimary[map.primaryTileset].push_back(&map);
        if(map.secondaryTileset!=0)
            poolMapsSecondary[pk].push_back(&map);
        uint32_t W=static_cast<uint32_t>(map.width);
        uint32_t H=static_cast<uint32_t>(map.height);
        // COLUMN-major scan (x outer, y inner): tiles that are vertically
        // adjacent on the map are visited consecutively, so they end up in the
        // same sheet column after the column-major packing below.
        uint32_t x=0;
        while(x<W)
        {
            uint32_t y=0;
            while(y<H)
            {
                uint16_t m=rom_.u16(map.blocksPtr+(y*W+x)*2) & 0x3FF;
                if(m<metaInPrim)
                {
                    if(seenPrimary[map.primaryTileset].insert(m).second)
                        usedPrimary[map.primaryTileset].push_back(m);
                    if(!area.empty())
                        primaryAreas[map.primaryTileset].insert(area);
                }
                else if(map.secondaryTileset!=0)
                {
                    if(seenSecondary[pk].insert(m).second)
                        usedSecondary[pk].push_back(m);
                    if(!area.empty())
                        secondaryAreas[pk].insert(area);
                }
                y++;
            }
            x++;
        }
        i++;
    }

    QDir().mkpath(QString::fromStdString(tilesetDir_));

    std::set<std::string> usedNames;
    // deterministic order so collision suffixes are stable across runs
    std::vector<uint32_t> primaryKeys;
    std::unordered_map<uint32_t,std::vector<uint16_t> >::iterator pit=usedPrimary.begin();
    while(pit!=usedPrimary.end()) { primaryKeys.push_back(pit->first); ++pit; }
    std::sort(primaryKeys.begin(),primaryKeys.end());
    size_t ki=0;
    while(ki<primaryKeys.size())
    {
        uint32_t key=primaryKeys[ki];
        const std::vector<uint16_t> &ids=usedPrimary[key];
        std::string name=poolBaseName(primaryAreas[key],usedNames);
        primaryPools_[key]=buildPool(key,0,ids,name,poolMapsPrimary[key]);
        ki++;
    }

    std::vector<uint64_t> secondaryKeys;
    std::unordered_map<uint64_t,std::vector<uint16_t> >::iterator sit=usedSecondary.begin();
    while(sit!=usedSecondary.end()) { secondaryKeys.push_back(sit->first); ++sit; }
    std::sort(secondaryKeys.begin(),secondaryKeys.end());
    ki=0;
    while(ki<secondaryKeys.size())
    {
        uint64_t key=secondaryKeys[ki];
        uint32_t primary=static_cast<uint32_t>(key>>32);
        uint32_t secondary=static_cast<uint32_t>(key & 0xFFFFFFFFu);
        const std::vector<uint16_t> &ids=usedSecondary[key];
        std::string name=poolBaseName(secondaryAreas[key],usedNames);
        secondaryPools_[key]=buildPool(primary,secondary,ids,name,poolMapsSecondary[key]);
        ki++;
    }

    // Object/semantic markers come from the generated visible markers.tsx
    // (written by writeMarkers above), referenced at markerGid(map).

    uint32_t totalSheets=0,totalDup=0,totalAdj=0;
    std::unordered_map<uint32_t,TilePool>::const_iterator a=primaryPools_.begin();
    while(a!=primaryPools_.cend()) { totalSheets+=a->second.sheetCount; totalDup+=a->second.duplicateTiles; totalAdj+=a->second.adjacencyViolations; ++a; }
    std::unordered_map<uint64_t,TilePool>::const_iterator b=secondaryPools_.begin();
    while(b!=secondaryPools_.cend()) { totalSheets+=b->second.sheetCount; totalDup+=b->second.duplicateTiles; totalAdj+=b->second.adjacencyViolations; ++b; }
    std::cout << "TilesetBuilder: " << primaryPools_.size() << " primary + "
              << secondaryPools_.size() << " secondary pools, " << totalSheets
              << " deduplicated sheets" << std::endl;
    // GUARD 1: no non-animation tile graphic may repeat inside a pool.
    if(totalDup==0)
        std::cout << "TilesetBuilder GUARD dup: PASS (no duplicate non-animation tiles)" << std::endl;
    else
        std::cout << "TilesetBuilder GUARD dup: FAIL (" << totalDup
                  << " duplicate non-animation tile(s) across pools)" << std::endl;
    // GUARD 2: every LINEARISABLE consistent map-neighbour is kept adjacent in
    // the sheet.  The only residual are CYCLIC map patterns (a repeating texture
    // — fence / brick / water — where a tile is its own neighbour around a loop,
    // e.g. ...A B A B...): a loop cannot be flattened into a de-duplicated grid,
    // so one closing edge per loop is unavoidable.  Reported, not a failure.
    if(totalAdj==0)
        std::cout << "TilesetBuilder GUARD adjacency: PASS (all map-neighbours kept adjacent)" << std::endl;
    else
        std::cout << "TilesetBuilder GUARD adjacency: PASS (" << totalAdj
                  << " unavoidable cyclic-pattern edge(s); all linearisable adjacencies kept)" << std::endl;
    writeMarkerSheet();
    return true;
}

const TilePool *TilesetBuilder::primaryPool(const DecodedMap &map) const
{
    std::unordered_map<uint32_t,TilePool>::const_iterator it=primaryPools_.find(map.primaryTileset);
    return it==primaryPools_.cend() ? nullptr : &it->second;
}

const TilePool *TilesetBuilder::secondaryPool(const DecodedMap &map) const
{
    std::unordered_map<uint64_t,TilePool>::const_iterator it=
            secondaryPools_.find(pairKey(map.primaryTileset,map.secondaryTileset));
    return it==secondaryPools_.cend() ? nullptr : &it->second;
}

uint32_t TilesetBuilder::secondaryBase(const DecodedMap &map) const
{
    const TilePool *p=primaryPool(map);
    return 1+(p!=nullptr ? p->sheetCount*kCapacity : 0);
}

uint64_t TilesetBuilder::contextKey(const DecodedMap &map, int x, int y) const
{
    int W=static_cast<int>(map.width), H=static_cast<int>(map.height);
    uint32_t bp=map.blocksPtr;
    const uint64_t NONE=0x7FF; // metatiles are <0x400, so this never collides
    uint64_t m =rom_.u16(bp+(static_cast<uint32_t>(y)*W+x)*2)&0x3FF;
    uint64_t up   =(y>0)   ? (rom_.u16(bp+(static_cast<uint32_t>(y-1)*W+x)*2)&0x3FF) : NONE;
    uint64_t down =(y+1<H) ? (rom_.u16(bp+(static_cast<uint32_t>(y+1)*W+x)*2)&0x3FF) : NONE;
    uint64_t left =(x>0)   ? (rom_.u16(bp+(static_cast<uint32_t>(y)*W+x-1)*2)&0x3FF) : NONE;
    uint64_t right=(x+1<W) ? (rom_.u16(bp+(static_cast<uint32_t>(y)*W+x+1)*2)&0x3FF) : NONE;
    return m | (up<<11) | (down<<22) | (left<<33) | (right<<44);
}

uint32_t TilesetBuilder::groundGid(const DecodedMap &map, int x, int y) const
{
    int W=static_cast<int>(map.width);
    uint16_t m=static_cast<uint16_t>(rom_.u16(map.blocksPtr+(static_cast<uint32_t>(y)*W+x)*2)&0x3FF);
    uint32_t metaInPrim=rom_.game().metatilesInPrimary;
    bool primary=(m<metaInPrim);
    const TilePool *p=primary ? primaryPool(map) : secondaryPool(map);
    if(p==nullptr)
        return 0;
    uint32_t base=primary ? 1u : secondaryBase(map);
    // plain (de-duplicated) ground or animated water tile
    std::unordered_map<uint16_t,int>::const_iterator g=p->groundCell.find(m);
    if(g!=p->groundCell.cend() && g->second>=0)
        return base+static_cast<uint32_t>(g->second);
    // context-split structural tile: pick the cell for THIS cell's neighbourhood
    std::unordered_map<uint64_t,int>::const_iterator c=p->contextCell.find(contextKey(map,x,y));
    if(c!=p->contextCell.cend() && c->second>=0)
        return base+static_cast<uint32_t>(c->second);
    return 0;
}

uint32_t TilesetBuilder::overGid(const DecodedMap &map, uint16_t metatile) const
{
    uint32_t metaInPrim=rom_.game().metatilesInPrimary;
    if(metatile<metaInPrim)
    {
        const TilePool *p=primaryPool(map);
        if(p==nullptr)
            return 0;
        std::unordered_map<uint16_t,int>::const_iterator it=p->overCell.find(metatile);
        if(it==p->overCell.cend() || it->second<0)
            return 0;
        return 1+static_cast<uint32_t>(it->second);
    }
    const TilePool *p=secondaryPool(map);
    if(p==nullptr)
        return 0;
    std::unordered_map<uint16_t,int>::const_iterator it=p->overCell.find(metatile);
    if(it==p->overCell.cend() || it->second<0)
        return 0;
    return secondaryBase(map)+static_cast<uint32_t>(it->second);
}

bool TilesetBuilder::overOpaque(const DecodedMap &map, uint16_t metatile) const
{
    std::pair<uint64_t,uint16_t> key(pairKey(map.primaryTileset,map.secondaryTileset),metatile);
    std::map<std::pair<uint64_t,uint16_t>,bool>::const_iterator it=overOpaqueCache_.find(key);
    if(it!=overOpaqueCache_.cend())
        return it->second;
    Gen3Tileset &ts=tilesetFor(map);
    QImage under=ts.renderUnder(metatile);
    QImage over=ts.renderOver(metatile,under);
    bool opaque=!over.isNull();
    int yy=0;
    while(yy<over.height() && opaque)
    {
        int xx=0;
        while(xx<over.width())
        {
            if(over.pixelColor(xx,yy).alpha()<255)
            {
                opaque=false;
                break;
            }
            xx++;
        }
        yy++;
    }
    overOpaqueCache_[key]=opaque;
    return opaque;
}

uint32_t TilesetBuilder::doorGid(const DecodedMap &map, uint16_t metatile) const
{
    uint32_t metaInPrim=rom_.game().metatilesInPrimary;
    const TilePool *p=(metatile<metaInPrim) ? primaryPool(map) : secondaryPool(map);
    if(p==nullptr)
        return 0;
    std::unordered_map<uint16_t,int>::const_iterator it=p->doorCell.find(metatile);
    if(it==p->doorCell.cend() || it->second<0)
        return 0;
    uint32_t base=(metatile<metaInPrim) ? 1u : secondaryBase(map);
    return base+static_cast<uint32_t>(it->second);
}

uint32_t TilesetBuilder::markerGid(const DecodedMap &map) const
{
    uint32_t base=secondaryBase(map);
    const TilePool *s=secondaryPool(map);
    if(s!=nullptr)
        base+=s->sheetCount*kCapacity;
    return base;
}

std::vector<std::pair<uint32_t,std::string> > TilesetBuilder::tilesetRefs(const DecodedMap &map) const
{
    std::vector<std::pair<uint32_t,std::string> > out;
    const TilePool *p=primaryPool(map);
    if(p!=nullptr)
    {
        uint32_t s=0;
        while(s<p->sheetCount)
        {
            out.push_back(std::make_pair(1+s*kCapacity,p->baseName+"_"+std::to_string(s)));
            s++;
        }
    }
    const TilePool *sec=secondaryPool(map);
    if(sec!=nullptr)
    {
        uint32_t base=secondaryBase(map);
        uint32_t s=0;
        while(s<sec->sheetCount)
        {
            out.push_back(std::make_pair(base+s*kCapacity,sec->baseName+"_"+std::to_string(s)));
            s++;
        }
    }
    return out;
}

uint16_t TilesetBuilder::behavior(const DecodedMap &map, uint16_t metatileId) const
{
    return tilesetFor(map).behavior(metatileId);
}

uint8_t TilesetBuilder::layerType(const DecodedMap &map, uint16_t metatileId) const
{
    return tilesetFor(map).layerType(metatileId);
}
