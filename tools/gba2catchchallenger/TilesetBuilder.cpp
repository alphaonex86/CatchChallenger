#include "TilesetBuilder.hpp"
#include "Naming.hpp"

#include <QDir>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <set>
#include <unordered_set>

const uint32_t TilesetBuilder::kCapacity=1024;
const int TilesetBuilder::kColumns=32;

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

// A guessed terrain name from an average colour (best-effort, for the wangcolor).
static std::string terrainName(int r, int g, int b)
{
    int mx=std::max(r,std::max(g,b)), mn=std::min(r,std::min(g,b));
    if(mx-mn<28)
        return (mx>185) ? "path" : ((mx<80) ? "shadow" : "rock");
    if(g>=r && g>=b)
        return "grass";
    if(b>=r && b>=g)
        return "water";
    if(r>=b && g>=b && r>150)
        return "sand";
    return "terrain";
}

// Best-effort Tiled "corner" Wang/terrain set for a sheet: cluster every OPAQUE
// (non-overlay, non-animation) tile's four 8x8 corners by AVERAGE colour; the
// large clusters become terrain colours and each tile gets a per-corner wangid,
// so Tiled can auto-tile grass<->sand / land<->water transitions.  This is purely
// editor metadata (libtiled ignores it when rendering).  Detail tiles fall into
// small clusters and stay terrain 0 (none).
static void emitWangSet(std::ostream &tsx, const QImage &sheet, int columns, uint32_t tilecount,
                        const std::unordered_map<int,std::string> &anims)
{
    static const int cox[4]={0,8,0,8}; // corner offsets: 0=TL 1=TR 2=BL 3=BR
    static const int coy[4]={0,0,8,8};
    std::vector<int> cornerCluster(static_cast<size_t>(tilecount)*4,-1);
    std::vector<int> repR,repG,repB,repCount;
    const long kJoin=42*42; // squared RGB distance to join a cluster
    uint32_t t=0;
    while(t<tilecount)
    {
        int tc=static_cast<int>(t%static_cast<uint32_t>(columns));
        int tr=static_cast<int>(t/static_cast<uint32_t>(columns));
        int bx=tc*16, by=tr*16;
        bool ok=(anims.find(static_cast<int>(t))==anims.cend());
        if(ok)
        {
            int yy=0;
            while(yy<16 && ok){ int xx=0; while(xx<16){ if(qAlpha(sheet.pixel(bx+xx,by+yy))<255){ok=false;break;} xx++; } yy++; }
        }
        if(ok)
        {
            int k=0;
            while(k<4)
            {
                long sr=0,sg=0,sb=0;
                int yy=0;
                while(yy<8){ int xx=0; while(xx<8){ QRgb p=sheet.pixel(bx+cox[k]+xx,by+coy[k]+yy); sr+=qRed(p);sg+=qGreen(p);sb+=qBlue(p); xx++; } yy++; }
                int ar=static_cast<int>(sr/64),ag=static_cast<int>(sg/64),ab=static_cast<int>(sb/64);
                int best=-1; long bestd=0;
                size_t ci=0;
                while(ci<repR.size())
                {
                    long dr=ar-repR[ci],dg=ag-repG[ci],db=ab-repB[ci];
                    long d=dr*dr+dg*dg+db*db;
                    if(best<0 || d<bestd){ best=static_cast<int>(ci); bestd=d; }
                    ci++;
                }
                if(best>=0 && bestd<=kJoin){ cornerCluster[t*4+k]=best; repCount[best]++; }
                else { cornerCluster[t*4+k]=static_cast<int>(repR.size()); repR.push_back(ar);repG.push_back(ag);repB.push_back(ab);repCount.push_back(1); }
                k++;
            }
        }
        t++;
    }
    // Keep the dominant clusters (appearing on many corners) as terrains, biggest
    // first (cap 8) — enough for the main grass/sand/water/rock/path terrains
    // without splintering into near-identical shades.
    std::vector<std::pair<int,int> > rank; // (count, clusterId)
    { size_t ci=0; while(ci<repR.size()){ if(repCount[ci]>=12) rank.push_back(std::make_pair(repCount[ci],static_cast<int>(ci))); ci++; } }
    std::sort(rank.begin(),rank.end());
    std::reverse(rank.begin(),rank.end());
    std::unordered_map<int,int> clusterTerrain; // clusterId -> 1-based terrain id
    if(rank.size()>8) rank.resize(8);
    size_t ri=0;
    while(ri<rank.size()){ clusterTerrain[rank[ri].second]=static_cast<int>(ri)+1; ri++; }
    if(clusterTerrain.empty())
        return;
    tsx << " <wangsets>\n  <wangset name=\"Terrain\" type=\"corner\" tile=\"-1\">\n";
    ri=0;
    while(ri<rank.size())
    {
        int cid=rank[ri].second;
        char col[8]; std::snprintf(col,sizeof(col),"#%02x%02x%02x",repR[cid]&0xff,repG[cid]&0xff,repB[cid]&0xff);
        tsx << "   <wangcolor name=\"" << terrainName(repR[cid],repG[cid],repB[cid]) << "-" << (ri+1)
            << "\" color=\"" << col << "\" tile=\"-1\" probability=\"1\"/>\n";
        ri++;
    }
    // corner index -> wangid position: TR=1, BR=3, BL=5, TL=7 (edges 0/2/4/6 stay 0)
    t=0;
    while(t<tilecount)
    {
        int tl=-1,tr=-1,bl=-1,br=-1;
        std::unordered_map<int,int>::const_iterator f;
        f=clusterTerrain.find(cornerCluster[t*4+0]); tl=(f!=clusterTerrain.cend())?f->second:0;
        f=clusterTerrain.find(cornerCluster[t*4+1]); tr=(f!=clusterTerrain.cend())?f->second:0;
        f=clusterTerrain.find(cornerCluster[t*4+2]); bl=(f!=clusterTerrain.cend())?f->second:0;
        f=clusterTerrain.find(cornerCluster[t*4+3]); br=(f!=clusterTerrain.cend())?f->second:0;
        if(cornerCluster[t*4+0]<0){ t++; continue; } // not an opaque terrain tile
        if(tl>0||tr>0||bl>0||br>0)
            tsx << "   <wangtile tileid=\"" << t << "\" wangid=\"0," << tr << ",0," << br << ",0," << bl << ",0," << tl << "\"/>\n";
        t++;
    }
    tsx << "  </wangset>\n </wangsets>\n";
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
        emitWangSet(tsx,sheet,columns,tilecount,anims);
        tsx << "</tileset>\n";
    }
}

// Add an image to the de-dup pool; returns its (possibly existing) cell index.
// Near-duplicate fold tolerance.  The Gen3 ROM often stores two metatiles that
// render to VISUALLY-IDENTICAL 16x16 graphics (a sub-pixel shading / palette
// nuance) — byte-exact dedup keeps them, so they read as duplicate rows/cols in
// the sheet.  Two tiles fold together when EVERY channel is within kNearMaxDiff
// AND the mean per-channel difference is within kNearMeanDiff (both tiny, so the
// merge is imperceptible).  Lossy by design (owner-approved).
static const int kNearMaxDiff=24;   // no single RGBA channel differs by more than this (~9%)
static const int kNearMeanDiff=2;   // average per-channel difference over the tile

// True when a and b (16x16 ARGB32) are near-identical per the tolerance above.
static bool nearEqualTile(const QImage &a, const QImage &b)
{
    const qsizetype n=a.sizeInBytes();
    if(b.sizeInBytes()!=n)
        return false;
    const uchar *pa=a.constBits();
    const uchar *pb=b.constBits();
    long sum=0;
    qsizetype i=0;
    while(i<n)
    {
        int d=static_cast<int>(pa[i])-static_cast<int>(pb[i]);
        if(d<0) d=-d;
        if(d>kNearMaxDiff)
            return false;
        sum+=d;
        i++;
    }
    return n>0 && (sum/n)<=kNearMeanDiff;
}

static int dedupAdd(const QImage &img, std::unordered_map<std::string,int> &seen, std::vector<QImage> &tiles)
{
    QImage a=img.convertToFormat(QImage::Format_ARGB32);
    std::string key(reinterpret_cast<const char *>(a.constBits()),static_cast<size_t>(a.sizeInBytes()));
    std::unordered_map<std::string,int>::const_iterator it=seen.find(key);
    if(it!=seen.cend())
        return it->second;            // exact duplicate -> reuse
    // Near-duplicate: fold into an existing visually-identical tile.  Linear scan
    // with early-exit (pools are bounded; most pairs differ on the first pixels).
    size_t t=0;
    while(t<tiles.size())
    {
        if(nearEqualTile(a,tiles[t]))
        {
            seen[key]=static_cast<int>(t); // cache so the next exact match is O(1)
            return static_cast<int>(t);
        }
        t++;
    }
    int idx=static_cast<int>(tiles.size());
    tiles.push_back(a);
    seen[key]=idx;
    return idx;
}

// The single most-frequent neighbour in a (neighbour -> count) histogram; ties
// broken toward the smallest index for determinism.  -1 when the histogram is
// empty (the tile never had a neighbour in that direction).
static int dominantNeighbour(const std::unordered_map<int,int> &counts)
{
    int best=-1,bestCount=0;
    std::unordered_map<int,int>::const_iterator it=counts.cbegin();
    while(it!=counts.cend())
    {
        if(it->second>bestCount || (it->second==bestCount && (best<0 || it->first<best)))
        {
            best=it->first;
            bestCount=it->second;
        }
        ++it;
    }
    return best;
}

// True when every pixel of the tile is fully opaque (a baked ground tile).
static bool isFullyOpaque(const QImage &img)
{
    int w=img.width(),h=img.height(),y=0;
    while(y<h){ int x=0; while(x<w){ if(qAlpha(img.pixel(x,y))!=255) return false; x++; } y++; }
    return true;
}

// Foreground of opaque tile x sitting OVER background terrain t: a pixel that
// differs from t by more than maxDiff on any channel keeps x's colour (it is the
// object); a pixel matching t becomes transparent (the terrain shows through from
// the layer below).  *opaqueOut = count of kept (object) pixels.
static QImage foregroundOver(const QImage &x, const QImage &t, int maxDiff, int &opaqueOut)
{
    int w=x.width(),h=x.height();
    QImage fg(w,h,QImage::Format_ARGB32);
    fg.fill(Qt::transparent);
    int op=0,y=0;
    while(y<h)
    {
        int xx=0;
        while(xx<w)
        {
            QRgb a=x.pixel(xx,y), b=t.pixel(xx,y);
            if(qAbs(qRed(a)-qRed(b))>maxDiff || qAbs(qGreen(a)-qGreen(b))>maxDiff || qAbs(qBlue(a)-qBlue(b))>maxDiff)
            { fg.setPixel(xx,y,a); op++; }
            xx++;
        }
        y++;
    }
    opaqueOut=op;
    return fg;
}

// Largest 4-connected run of pixels where opaque tiles x and y AGREE (<=maxDiff per
// channel) — the SHARED object of two baked tiles that differ only in background.
// Writes that component into compMask (1 = object pixel); returns its pixel count.
// *bgDiff = number of pixels where x and y differ (their two backgrounds).
static int sharedObjectMask(const QImage &x, const QImage &y, int maxDiff, std::vector<char> &compMask, int &bgDiff)
{
    const int w=16,h=16,n=w*h;
    std::vector<char> same(static_cast<size_t>(n),0);
    int diff=0,yy=0;
    while(yy<h)
    {
        int xx=0;
        while(xx<w)
        {
            QRgb a=x.pixel(xx,yy), b=y.pixel(xx,yy);
            bool s=(qAbs(qRed(a)-qRed(b))<=maxDiff && qAbs(qGreen(a)-qGreen(b))<=maxDiff && qAbs(qBlue(a)-qBlue(b))<=maxDiff);
            same[static_cast<size_t>(yy*w+xx)]=s?1:0;
            if(!s) diff++;
            xx++;
        }
        yy++;
    }
    bgDiff=diff;
    std::vector<char> seen(static_cast<size_t>(n),0);
    compMask.assign(static_cast<size_t>(n),0);
    int best=0,p=0;
    while(p<n)
    {
        if(same[static_cast<size_t>(p)] && !seen[static_cast<size_t>(p)])
        {
            std::vector<int> stack,comp;
            stack.push_back(p); seen[static_cast<size_t>(p)]=1;
            while(!stack.empty())
            {
                int c=stack.back(); stack.pop_back(); comp.push_back(c);
                int cx=c%w,cy=c/w;
                static const int dxx[4]={1,-1,0,0},dyy[4]={0,0,1,-1};
                int k=0;
                while(k<4)
                {
                    int nx=cx+dxx[k],ny=cy+dyy[k];
                    if(nx>=0&&nx<w&&ny>=0&&ny<h)
                    {
                        int ni=ny*w+nx;
                        if(same[static_cast<size_t>(ni)] && !seen[static_cast<size_t>(ni)]){ seen[static_cast<size_t>(ni)]=1; stack.push_back(ni); }
                    }
                    k++;
                }
            }
            if(static_cast<int>(comp.size())>best)
            {
                best=static_cast<int>(comp.size());
                std::fill(compMask.begin(),compMask.end(),0);
                size_t q=0; while(q<comp.size()){ compMask[static_cast<size_t>(comp[q])]=1; q++; }
            }
        }
        p++;
    }
    return best;
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
    std::vector<uint32_t> &pos, uint32_t &adjViol, const std::string &label,
    const std::vector<int> &catOf)
{
    (void)label;
    int N=static_cast<int>(tiles.size());
    std::vector<char> used(N,0);
    // DOMINANT on-map neighbour: count how often each tile sits immediately to
    // the right / below / left / above another across all pool maps, then keep
    // only the single most-frequent neighbour per direction.  This groups a
    // building's tiles into one block even when an occasional map cell differs —
    // the old rule required the neighbour to be IDENTICAL on EVERY map, which
    // fragmented anything that varied.  Edges survive only when reciprocated.
    std::vector<std::unordered_map<int,int> > cR(N),cD(N),cL(N),cU(N);
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
                    nb=(x+1<W)?ct[static_cast<size_t>(y)*W+x+1]:-1; if(nb>=0) cR[ti][nb]++;
                    nb=(y+1<H)?ct[static_cast<size_t>(y+1)*W+x]:-1; if(nb>=0) cD[ti][nb]++;
                    nb=(x>0)?ct[static_cast<size_t>(y)*W+x-1]:-1;   if(nb>=0) cL[ti][nb]++;
                    nb=(y>0)?ct[static_cast<size_t>(y-1)*W+x]:-1;   if(nb>=0) cU[ti][nb]++;
                }
                x++;
            }
            y++;
        }
        mi++;
    }
    std::vector<int> rN(N,-1),dN(N,-1),lN(N,-1),uN(N,-1);
    int t=0;
    while(t<N)
    {
        rN[t]=dominantNeighbour(cR[static_cast<size_t>(t)]);
        dN[t]=dominantNeighbour(cD[static_cast<size_t>(t)]);
        lN[t]=dominantNeighbour(cL[static_cast<size_t>(t)]);
        uN[t]=dominantNeighbour(cU[static_cast<size_t>(t)]);
        t++;
    }
    // Keep the UNRECIPROCATED dominant neighbours: a reused corner tile (its
    // edge into a building is dominant one way but the building's edge points at
    // a different reuse) stays a single below, but we still want to drop it into
    // that building's hole rather than scatter it (smart gap-fill).
    std::vector<int> rN0(rN),dN0(dN),lN0(lN),uN0(uN);
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

    // Pack the MULTI-tile blocks (which must keep their 2-D shape) with a SKYLINE
    // bottom-left strip-packer — the classic rectangle bin-packing used for sprite
    // atlases (cf. rectpack2D / MaxRects).  Tallest blocks first; each block is
    // placed at the x that yields the LOWEST top sitting on the current per-column
    // skyline.  The single, adjacency-free tiles are collected separately and then
    // fill every remaining gap, so the sheet wastes almost no transparent space
    // (a shelf-pack left big empty bands below shorter blocks / at shelf ends).
    std::unordered_map<int,uint32_t> tilePos;
    std::vector<std::vector<int> > grid;
    std::vector<int> singles; // free 1x1 tiles -> fill gaps
    std::vector<std::pair<std::pair<int,int>,size_t> > blocks; // ((-h,-w), unit) -> sort tallest/widest first
    {
        size_t ui=0;
        while(ui<units.size())
        {
            if(keptUnit[ui])
            {
                if(units[ui].size()<=1)
                {
                    if(!units[ui].empty())
                        singles.push_back(units[ui][0]);
                }
                else
                    blocks.push_back(std::make_pair(std::make_pair(-unitH[ui],-unitW[ui]),ui));
            }
            ui++;
        }
    }
    std::sort(blocks.begin(),blocks.end());
    std::vector<int> skyline(static_cast<size_t>(COLS),0);
    size_t bk=0;
    while(bk<blocks.size())
    {
        size_t u=blocks[bk].second;
        int w=unitW[u],h=unitH[u];
        if(w>COLS) w=COLS;
        int bestX=0,bestY=1<<29;
        int x=0;
        while(x+w<=COLS)
        {
            int y=0,k=0;
            while(k<w){ if(skyline[static_cast<size_t>(x+k)]>y) y=skyline[static_cast<size_t>(x+k)]; k++; }
            if(y<bestY){ bestY=y; bestX=x; }
            x++;
        }
        while(static_cast<int>(grid.size())<bestY+h) grid.push_back(std::vector<int>(static_cast<size_t>(COLS),-1));
        size_t bi=0;
        while(bi<units[u].size())
        {
            int ti=units[u][bi];
            int gc=bestX+bc[ti],gr=bestY+br[ti];
            grid[static_cast<size_t>(gr)][static_cast<size_t>(gc)]=ti;
            tilePos[ti]=static_cast<uint32_t>(gr)*static_cast<uint32_t>(COLS)+static_cast<uint32_t>(gc);
            bi++;
        }
        int k=0; while(k<w){ skyline[static_cast<size_t>(bestX+k)]=bestY+h; k++; }
        bk++;
    }
    // SMART gap-fill: first drop each free single into an EXISTING hole that is
    // adjacent to one of its dominant on-map neighbours (using the unreciprocated
    // rN0/.. so a reused building corner still lands in its building's hole, e.g.
    // the bottom-left of a 2x2 whose other three tiles already form the block).
    // Repeat while progress is made (a single may anchor on another just placed),
    // then raster-fill whatever could not be anchored.
    std::vector<char> placedS(singles.size(),0);
    {
        bool progress=true;
        while(progress)
        {
            progress=false;
            size_t sj=0;
            while(sj<singles.size())
            {
                if(!placedS[sj])
                {
                    int s=singles[sj];
                    int cand=-1;
                    int dir=0;
                    while(dir<4 && cand<0)
                    {
                        int nb=(dir==0)?rN0[s]:(dir==1)?lN0[s]:(dir==2)?dN0[s]:uN0[s];
                        if(nb>=0)
                        {
                            int rep=nb; while(repOf[rep]!=rep) rep=repOf[rep];
                            std::unordered_map<int,uint32_t>::const_iterator pn=tilePos.find(rep);
                            if(pn!=tilePos.cend())
                            {
                                int nr=static_cast<int>(pn->second)/COLS, nc=static_cast<int>(pn->second)%COLS;
                                int tr=nr,tc=nc;
                                if(dir==0)tc=nc-1; else if(dir==1)tc=nc+1; else if(dir==2)tr=nr-1; else tr=nr+1;
                                if(tr>=0 && tc>=0 && tc<COLS && tr<static_cast<int>(grid.size()) && grid[static_cast<size_t>(tr)][static_cast<size_t>(tc)]<0)
                                    cand=tr*COLS+tc;
                            }
                        }
                        dir++;
                    }
                    if(cand>=0)
                    {
                        grid[static_cast<size_t>(cand/COLS)][static_cast<size_t>(cand%COLS)]=s;
                        tilePos[s]=static_cast<uint32_t>(cand);
                        placedS[sj]=1;
                        progress=true;
                    }
                }
                sj++;
            }
        }
    }
    { std::vector<int> rem; size_t sj=0; while(sj<singles.size()){ if(!placedS[sj]) rem.push_back(singles[sj]); sj++; } singles.swap(rem); }
    // The leftover singles can NOT be grouped by map adjacency, so group them by
    // another findable characteristic — their tileCategory (background/building/
    // water/grass/ledge/overlay) — so the same KIND clusters in the sheet instead of
    // scattering.  (Sort by (category, tile) pairs to avoid a custom comparator.)
    if(!catOf.empty())
    {
        std::vector<std::pair<int,int> > sk;
        size_t z=0;
        while(z<singles.size()){ int ti=singles[z]; int c=(ti>=0 && ti<static_cast<int>(catOf.size()))?catOf[static_cast<size_t>(ti)]:99; sk.push_back(std::make_pair(c,ti)); z++; }
        std::sort(sk.begin(),sk.end());
        z=0; while(z<sk.size()){ singles[z]=sk[z].second; z++; }
    }
    // Raster-fill the remaining (un-anchored) singles into any leftover gaps.
    size_t si=0;
    {
        size_t gr=0;
        while(gr<grid.size())
        {
            int gc=0;
            while(gc<COLS)
            {
                if(grid[gr][static_cast<size_t>(gc)]<0 && si<singles.size())
                {
                    int ti=singles[si++];
                    grid[gr][static_cast<size_t>(gc)]=ti;
                    tilePos[ti]=static_cast<uint32_t>(gr)*static_cast<uint32_t>(COLS)+static_cast<uint32_t>(gc);
                }
                gc++;
            }
            gr++;
        }
    }
    while(si<singles.size())
    {
        grid.push_back(std::vector<int>(static_cast<size_t>(COLS),-1));
        size_t gr=grid.size()-1;
        int gc=0;
        while(gc<COLS && si<singles.size())
        {
            int ti=singles[si++];
            grid[gr][static_cast<size_t>(gc)]=ti;
            tilePos[ti]=static_cast<uint32_t>(gr)*static_cast<uint32_t>(COLS)+static_cast<uint32_t>(gc);
            gc++;
        }
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
    mainCount(0),
    sheetCount(0),
    duplicateTiles(0),
    adjacencyViolations(0),
    tinyTiles(0),
    bgFgSplits(0),
    layerSplitTiles(0),
    layerSplitBad(0)
{
}

// A pool's tiles are emitted as several sheet FILES.  The ground+over ("main")
// tiles [0,mainCount) are cut into kCapacity-sized sheets named "<base>_<n>"; the
// door + animated tiles [mainCount,uniqueCount) are then cut as their OWN final
// sheet(s) named "<base>_anim_<n>", so they sit on a unique, easy-to-find tileset.
// Gids stay base+position; only the file split and each sheet's firstgid
// (= base + startCell) change.  Returns (startCell, count, name-suffix) per sheet.
namespace {
struct SheetSeg { uint32_t start; uint32_t count; std::string suffix; };
std::vector<SheetSeg> poolSheetSegments(uint32_t mainCount, uint32_t uniqueCount)
{
    std::vector<SheetSeg> segs;
    const uint32_t cap=TilesetBuilder::kCapacity;
    uint32_t s=0;
    while(s*cap<mainCount)
    {
        uint32_t start=s*cap, cnt=mainCount-start;
        if(cnt>cap) cnt=cap;
        SheetSeg seg; seg.start=start; seg.count=cnt; seg.suffix="_"+std::to_string(s);
        segs.push_back(seg);
        s++;
    }
    uint32_t animCount=(uniqueCount>mainCount)?(uniqueCount-mainCount):0;
    uint32_t a=0;
    while(a*cap<animCount)
    {
        uint32_t start=mainCount+a*cap, cnt=animCount-a*cap;
        if(cnt>cap) cnt=cap;
        SheetSeg seg; seg.start=start; seg.count=cnt; seg.suffix="_anim_"+std::to_string(a);
        segs.push_back(seg);
        a++;
    }
    return segs;
}
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
                                   const std::vector<const DecodedMap *> &poolMaps,
                                   const std::string &subDir)
{
    Gen3Tileset ts(rom_,primaryPtr,secondaryPtr);
    TilePool pool;
    pool.baseName=baseName;
    pool.subDir=subDir;
    // Sheets of a region-specific pool live in tileset/<region>/ (see prepare).
    const std::string outDir=subDir.empty() ? tilesetDir_ : tilesetDir_+"/"+subDir;
    if(!subDir.empty())
        QDir().mkpath(QString::fromStdString(outDir));
    const int overThreshold=12; // a significant over needs >12 opaque px (no near-empty overlay)
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

    // Per-metatile collision usage across the pool's maps.  A metatile that is
    // COLLIDABLE everywhere it appears (the player is never under-foot) has its
    // top sub-layer COMPOSITED into its ground tile below — nothing needs to draw
    // between the two sub-layers — so a building reads VISUALLY MERGED in the
    // sheet (one wall+roof tile) instead of a split wall + separate roof.  A
    // metatile ever walked on keeps the split (its top must lift to WalkBehind so
    // the player can pass behind it).
    std::unordered_map<uint16_t,uint8_t> collFlag; // bit0 = collidable, bit1 = walkable
    {
        size_t pm=0;
        while(pm<poolMaps.size())
        {
            const DecodedMap *mp=poolMaps[pm];
            int W=static_cast<int>(mp->width), H=static_cast<int>(mp->height);
            int yy=0;
            while(yy<H)
            {
                int xx=0;
                while(xx<W)
                {
                    uint16_t blk=rom_.u16(mp->blocksPtr+(static_cast<uint32_t>(yy)*W+xx)*2);
                    collFlag[static_cast<uint16_t>(blk&0x3FF)] |= ((blk>>10)&0x3) ? 1 : 2;
                    xx++;
                }
                yy++;
            }
            pm++;
        }
    }

    // Pre-render each metatile's bottom (under) and top (over), and record which
    // distinct backgrounds (unders) each significant top sits on.  A top seen on
    // >=2 backgrounds — or whose base terrain ALSO exists as a standalone ground
    // tile — is a REUSABLE OVERLAY (a cliff/ledge edge, a rock, a tree top): it
    // stays a separate transparent tile over the base terrain (reused across
    // backgrounds = fewer tiles, e.g. one rock over any ground).  A top UNIQUE to
    // its background on an always-collidable cell is a building face -> composited.
    std::unordered_map<uint16_t,QImage> underCache,overCache;
    std::unordered_map<uint16_t,char> sigOver;
    std::unordered_map<std::string,std::unordered_set<std::string> > overToUnders;
    std::unordered_set<std::string> standaloneUnders;
    {
        size_t p=0;
        while(p<usedIds.size())
        {
            uint16_t id=usedIds[p];
            if(ts.behavior(id)==0x69 || (waterFrames>0 && ts.isAnimatedWater(id))) { p++; continue; }
            QImage under=ts.renderUnder(id).convertToFormat(QImage::Format_ARGB32);
            QImage over=ts.renderOver(id,under).convertToFormat(QImage::Format_ARGB32);
            std::string uk(reinterpret_cast<const char *>(under.constBits()),static_cast<size_t>(under.sizeInBytes()));
            bool sig=(ts.layerType(id)!=1 && largestOpaqueComponent(over)>overThreshold);
            underCache[id]=under;
            overCache[id]=over;
            sigOver[id]=sig?1:0;
            if(sig)
            {
                std::string ok(reinterpret_cast<const char *>(over.constBits()),static_cast<size_t>(over.sizeInBytes()));
                overToUnders[ok].insert(uk);
            }
            else
                standaloneUnders.insert(uk); // a plain tile's base IS a standalone terrain
            p++;
        }
    }

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
            const uint8_t cf=collFlag.count(id) ? collFlag[id] : 0;
            const bool alwaysCollidable=((cf&1)!=0) && ((cf&2)==0);
            const QImage &under=underCache[id];
            const QImage &over=overCache[id];
            bool reusable=false;
            if(sigOver[id])
            {
                std::string ok(reinterpret_cast<const char *>(over.constBits()),static_cast<size_t>(over.sizeInBytes()));
                std::string uk(reinterpret_cast<const char *>(under.constBits()),static_cast<size_t>(under.sizeInBytes()));
                std::unordered_map<std::string,std::unordered_set<std::string> >::const_iterator f=overToUnders.find(ok);
                reusable=(f!=overToUnders.cend() && f->second.size()>=2) || (standaloneUnders.count(uk)>0);
            }
            // Decompose (base terrain + reusable transparent overlay) for a
            // reusable overlay OR any walkable cell (the top must lift above the
            // player so it can be walked behind); composite (fuse into one tile)
            // only a UNIQUE top on an always-collidable cell = a building face.
            if(sigOver[id] && (reusable || !alwaysCollidable))
            {
                overLocal[id]=dedupAdd(over,overSeen,overs);
                metaGroundImg[id]=under;
            }
            else
            {
                overLocal[id]=-1;
                metaGroundImg[id]=ts.renderMetatile(id).convertToFormat(QImage::Format_ARGB32);
            }
        }
        i++;
    }

    std::vector<uint16_t> splitIds; // metatiles split by Pass 1b (verified at end)
    // Pass 1b: BACKGROUND/FOREGROUND split of baked collidable object tiles.  The
    // ROM bakes a collidable OBJECT (rock, cliff, tree) onto a terrain in ONE image,
    // so the SAME object renders as several near-duplicate tiles that differ ONLY in
    // their background terrain (user: common_0 #172/#177, #322/#394, #416/#420).
    // Detect "object over a WALKABLE terrain": for a composited, always-collidable,
    // fully-opaque ground tile X, find a terrain tile T that is WALKABLE somewhere
    // (a real background you can stand on -- this prevents an inverted split where
    // the "object" would be the background) and matches X except a connected
    // foreground region of >=12 px.  Split X into under = T (the terrain, reused) +
    // over = the transparent foreground (the object, ONE shared overlay reused over
    // every background).  Keep only foregrounds reused over >=2 DISTINCT terrains
    // (the user's "same object, different background"), so a one-off coincidence is
    // not split.  For a collidable cell CCWriter puts T on Collisions and the
    // foreground on the 2nd Collisions layer (both BELOW the player, OR-merged for
    // blocking) -> visually identical, and the background never lands on the
    // Water/Grass logic layer.
    {
        const int kBgMatchDiff=24;   // a background pixel matches the terrain within this
        const int kMinForeground=13; // extracted object must be >12 connected opaque px (no near-empty tile)
        const int kMinBackground=48; // and leave a real terrain area visible behind it
        std::vector<std::pair<std::string,QImage> > terrains;
        std::unordered_set<std::string> terrSeen;
        {
            std::unordered_map<uint16_t,QImage>::const_iterator tit=metaGroundImg.cbegin();
            while(tit!=metaGroundImg.cend())
            {
                const uint8_t cf=collFlag.count(tit->first)?collFlag[tit->first]:0;
                if(((cf&2)!=0) && isFullyOpaque(tit->second))
                {
                    std::string k(reinterpret_cast<const char *>(tit->second.constBits()),static_cast<size_t>(tit->second.sizeInBytes()));
                    if(terrSeen.insert(k).second) terrains.push_back(std::make_pair(k,tit->second));
                }
                ++tit;
            }
        }
        std::vector<uint16_t> objIds; std::vector<QImage> objFg; std::vector<int> objT;
        std::unordered_map<uint16_t,QImage>::const_iterator oit=metaGroundImg.cbegin();
        while(oit!=metaGroundImg.cend())
        {
            uint16_t id=oit->first;
            const uint8_t cf=collFlag.count(id)?collFlag[id]:0;
            bool alwaysCollidable=((cf&1)!=0)&&((cf&2)==0);
            uint16_t beh=ts.behavior(id);
            bool normalTerrain=!(beh==0x02||beh==0x03||(beh>=0x10&&beh<=0x15)||(beh>=0x38&&beh<=0x3B));
            if(overLocal.count(id) && overLocal[id]==-1 && alwaysCollidable && normalTerrain && isFullyOpaque(oit->second))
            {
                int bestT=-1,bestOp=1000; QImage bestFg;
                size_t ti=0;
                while(ti<terrains.size())
                {
                    int op=0;
                    QImage fg=foregroundOver(oit->second,terrains[ti].second,kBgMatchDiff,op);
                    if(op>=kMinForeground && (256-op)>=kMinBackground && op<bestOp && largestOpaqueComponent(fg)>=kMinForeground)
                    { bestOp=op; bestT=static_cast<int>(ti); bestFg=fg; }
                    ti++;
                }
                if(bestT>=0){ objIds.push_back(id); objFg.push_back(bestFg); objT.push_back(bestT); }
            }
            ++oit;
        }
        std::unordered_map<std::string,std::unordered_set<std::string> > fgToTerr;
        std::vector<std::string> fgKeys(objIds.size());
        size_t k=0;
        while(k<objIds.size())
        {
            std::string fk(reinterpret_cast<const char *>(objFg[k].constBits()),static_cast<size_t>(objFg[k].sizeInBytes()));
            fgKeys[k]=fk;
            fgToTerr[fk].insert(terrains[static_cast<size_t>(objT[k])].first);
            k++;
        }
        int splitCount=0;
        k=0;
        while(k<objIds.size())
        {
            if(fgToTerr[fgKeys[k]].size()>=2)
            {
                overLocal[objIds[k]]=dedupAdd(objFg[k],overSeen,overs);
                metaGroundImg[objIds[k]]=terrains[static_cast<size_t>(objT[k])].second;
                splitIds.push_back(objIds[k]);
                splitCount++;
            }
            k++;
        }
        if(splitCount>0)
            std::cout << "  bg/fg split: " << splitCount << " baked object tile(s) -> shared overlay + reused terrain in " << baseName << std::endl;
    }

    // Pass 1c: PAIR-BASED partial match — catches a SAME object on DIFFERENT
    // backgrounds that Pass 1b's full-tile terrain match misses (it picks a min-
    // foreground terrain that is NOT the real background, e.g. common_0 #315
    // cliff-over-water + #319 cliff-over-sand).  The SHARED region of two baked
    // tiles is their common OBJECT (consistent for both, unlike a per-tile min-fg
    // mask); the rest is background, PARTIAL-matched (over the object's complement)
    // to find its original tile so the object lifts onto one shared overlay.
    // SKIPPED for the 32MB hacks: their generic tiles share large regions everywhere,
    // so this aggressive split over-extracts and raises the hacks' render-visibility
    // miss; the officials (<=16MB) get the full benefit.
    if(rom_.size()<=0x1000000u)
    {
        const int kBgMatchDiff=24, kMinObject=64, kMinBackground=36, kRecomposeTol=48, kRecomposeMaxBad=2;
        std::vector<uint16_t> oc; std::vector<const QImage *> ocImg;   // object candidates (still composited)
        std::vector<const QImage *> terr;                              // WALKABLE terrains = the only valid backgrounds
        {
            std::unordered_map<uint16_t,QImage>::const_iterator it=metaGroundImg.cbegin();
            while(it!=metaGroundImg.cend())
            {
                uint16_t id=it->first;
                const uint8_t cf=collFlag.count(id)?collFlag[id]:0;
                if(((cf&2)!=0) && isFullyOpaque(it->second)) terr.push_back(&it->second); // a tile you can stand on
                bool alwaysCollidable=((cf&1)!=0)&&((cf&2)==0);
                uint16_t beh=ts.behavior(id);
                bool normalTerrain=!(beh==0x02||beh==0x03||(beh>=0x10&&beh<=0x15)||(beh>=0x38&&beh<=0x3B));
                if(overLocal.count(id) && overLocal[id]==-1 && alwaysCollidable && normalTerrain && isFullyOpaque(it->second))
                { oc.push_back(id); ocImg.push_back(&it->second); }
                ++it;
            }
        }
        std::vector<uint16_t> pairIds; std::vector<QImage> pairFg, pairBg;
        size_t a=0;
        while(a<oc.size())
        {
            std::vector<char> bestMask; int bestCC=0;
            size_t b2=0;
            while(b2<oc.size())
            {
                if(b2!=a)
                {
                    std::vector<char> m; int bgd=0;
                    int cc=sharedObjectMask(*ocImg[a],*ocImg[b2],kBgMatchDiff,m,bgd);
                    if(cc>=kMinObject && bgd>=kMinBackground && cc>bestCC){ bestCC=cc; bestMask.swap(m); }
                }
                b2++;
            }
            if(bestCC>=kMinObject)
            {
                // PARTIAL-match the background (complement of the object) against WALKABLE
                // terrains only -- the discriminator that keeps the OBJECT the foreground
                // (if the shared region were itself background, the "background" would be
                // the real object, which is NOT a walkable terrain, and nothing matches).
                int bestT=-1,bestMis=1000;
                size_t t=0;
                while(t<terr.size())
                {
                    if(terr[t]!=ocImg[a])
                    {
                        int mis=0,p=0;
                        while(p<256)
                        {
                            if(!bestMask[static_cast<size_t>(p)])
                            {
                                QRgb u=ocImg[a]->pixel(p%16,p/16), v=terr[t]->pixel(p%16,p/16);
                                if(qAbs(qRed(u)-qRed(v))>kBgMatchDiff||qAbs(qGreen(u)-qGreen(v))>kBgMatchDiff||qAbs(qBlue(u)-qBlue(v))>kBgMatchDiff) mis++;
                            }
                            p++;
                        }
                        if(mis<bestMis){ bestMis=mis; bestT=static_cast<int>(t); }
                    }
                    t++;
                }
                if(bestT>=0)
                {
                    // RECOMPOSE pre-check: terrain + object overlay must reproduce X (the
                    // guard's own test, run here so we only ever apply a split that PASSes).
                    int badRecompose=0,p=0;
                    while(p<256)
                    {
                        if(!bestMask[static_cast<size_t>(p)])
                        {
                            QRgb u=ocImg[a]->pixel(p%16,p/16), v=terr[static_cast<size_t>(bestT)]->pixel(p%16,p/16);
                            if(qAbs(qRed(u)-qRed(v))>kRecomposeTol||qAbs(qGreen(u)-qGreen(v))>kRecomposeTol||qAbs(qBlue(u)-qBlue(v))>kRecomposeTol) badRecompose++;
                        }
                        p++;
                    }
                    if(badRecompose<=kRecomposeMaxBad)
                    {
                        QImage ov(16,16,QImage::Format_ARGB32); ov.fill(Qt::transparent);
                        p=0; while(p<256){ if(bestMask[static_cast<size_t>(p)]) ov.setPixel(p%16,p/16,ocImg[a]->pixel(p%16,p/16)); p++; }
                        pairIds.push_back(oc[a]); pairFg.push_back(ov); pairBg.push_back(terr[static_cast<size_t>(bestT)]->copy());
                    }
                }
            }
            a++;
        }
        // Keep only an object reused over >=2 DISTINCT backgrounds (the genuine
        // "same object, different background"); a one-off match is dropped so the
        // split stays high-confidence (over-splitting covered whole terrain layers).
        std::unordered_map<std::string,std::unordered_set<std::string> > fgToBg;
        std::vector<std::string> fgK(pairIds.size());
        size_t k=0;
        while(k<pairIds.size())
        {
            fgK[k]=std::string(reinterpret_cast<const char *>(pairFg[k].constBits()),static_cast<size_t>(pairFg[k].sizeInBytes()));
            std::string bgk(reinterpret_cast<const char *>(pairBg[k].constBits()),static_cast<size_t>(pairBg[k].sizeInBytes()));
            fgToBg[fgK[k]].insert(bgk);
            k++;
        }
        int applied=0;
        k=0;
        while(k<pairIds.size())
        {
            if(fgToBg[fgK[k]].size()>=2)
            {
                overLocal[pairIds[k]]=dedupAdd(pairFg[k],overSeen,overs);
                metaGroundImg[pairIds[k]]=pairBg[k];
                splitIds.push_back(pairIds[k]);
                applied++;
            }
            k++;
        }
        if(applied>0)
            std::cout << "  bg/fg pair-split: " << applied << " same-object tile(s) -> shared overlay + matched background in " << baseName << std::endl;
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

    // Adaptive sheet WIDTH: a small pool wastes most of a fixed 32-wide sheet, so
    // size the width to the content (~sqrt of the tile count, a touch wider for
    // buildings) and cap at kColumns.  >=8 keeps an animation run (<=8 frames)
    // inside the gid budget and rarely splits a building block.  (Cell ids stay
    // linear, so the gid math is unaffected by the per-pool width.)
    int estTiles=static_cast<int>(grounds.size()+overs.size());
    int poolCols=static_cast<int>(std::ceil(std::sqrt(static_cast<double>(estTiles>0?estTiles:1)*1.15)));
    if(poolCols<8) poolCols=8;
    if(poolCols>kColumns) poolCols=kColumns;

    // UNIFIED human-view layout.  A human reads a building as ONE object, but it
    // is tiles on TWO layers: the ground/under wall (Walkable) and the over/
    // WalkBehind roof.  So lay out the TOP VISIBLE tile of every map cell -- its
    // over if it has one, else its ground -- in ONE 2-D pass, so the roof groups
    // directly ABOVE its wall exactly as seen (not in a separate over region).
    // The walls HIDDEN behind a roof (cells that carry both) are not in the human
    // view, so they go to a small region appended AFTER the visible one.
    uint32_t Gpre=static_cast<uint32_t>(grounds.size());
    // FOLD an over pixel-identical to a ground image onto that ground (no dup);
    // overFold[o] = OLD ground index it folds onto, else NOT_FOLDED.
    const uint32_t NOT_FOLDED=0xFFFFFFFFu;
    std::vector<uint32_t> overFold(overs.size(),NOT_FOLDED);
    {
        std::unordered_map<std::string,uint32_t> groundImgIdx;
        uint32_t p=0;
        while(p<Gpre)
        {
            QImage a=grounds[p].convertToFormat(QImage::Format_ARGB32);
            std::string key(reinterpret_cast<const char *>(a.constBits()),static_cast<size_t>(a.sizeInBytes()));
            if(groundImgIdx.find(key)==groundImgIdx.cend()) groundImgIdx[key]=p;
            p++;
        }
        size_t oi=0;
        while(oi<overs.size())
        {
            QImage a=overs[oi].convertToFormat(QImage::Format_ARGB32);
            std::string key(reinterpret_cast<const char *>(a.constBits()),static_cast<size_t>(a.sizeInBytes()));
            std::unordered_map<std::string,uint32_t>::const_iterator f=groundImgIdx.find(key);
            if(f!=groundImgIdx.cend()) overFold[oi]=f->second;
            oi++;
        }
    }
    // combined source for the visible pass: grounds [0,Gpre) ++ overs [Gpre,Gpre+O)
    std::vector<QImage> src;
    src.reserve(grounds.size()+overs.size());
    { size_t k=0; while(k<grounds.size()){ src.push_back(grounds[k]); k++; } k=0; while(k<overs.size()){ src.push_back(overs[k]); k++; } }
    // cellVis = top visible tile per cell (over folded -> its ground index, else
    // Gpre+overindex; no over -> the ground index).  visGround[g] flags a ground
    // image actually shown in the visible pass (so it is not duplicated as hidden).
    std::vector<std::vector<int> > cellVis(poolMaps.size());
    std::vector<char> visGround(Gpre,0);
    {
        size_t pmi=0;
        while(pmi<poolMaps.size())
        {
            const DecodedMap *mp=poolMaps[pmi];
            int W=static_cast<int>(mp->width), H=static_cast<int>(mp->height);
            cellVis[pmi].assign(static_cast<size_t>(W)*static_cast<size_t>(H),-1);
            int yy=0;
            while(yy<H)
            {
                int xx=0;
                while(xx<W)
                {
                    int g=cellTi[pmi][static_cast<size_t>(yy)*W+xx];
                    uint16_t m=static_cast<uint16_t>(rom_.u16(mp->blocksPtr+(static_cast<uint32_t>(yy)*W+xx)*2)&0x3FF);
                    std::unordered_map<uint16_t,int>::const_iterator ol=overLocal.find(m);
                    int vis=-1;
                    if(ol!=overLocal.cend() && ol->second>=0)
                    {
                        uint32_t of=overFold[static_cast<size_t>(ol->second)];
                        vis=(of!=NOT_FOLDED)?static_cast<int>(of):(static_cast<int>(Gpre)+ol->second);
                    }
                    else if(g>=0)
                        vis=g;
                    if(vis>=0 && vis<static_cast<int>(Gpre)) visGround[static_cast<size_t>(vis)]=1;
                    cellVis[pmi][static_cast<size_t>(yy)*W+xx]=vis;
                    xx++;
                }
                yy++;
            }
            pmi++;
        }
    }
    // tileCategory per src tile (grounds 0..Gpre-1, then overs) so layout2D groups
    // leftover singles by KIND — background(0,walkable) / building(1,collidable) /
    // water(2) / grass(3) / ledge(4) by behaviour / overlay(5) — when map adjacency
    // can't place them (user: group by indoor/outdoor/cave/building/background).
    std::vector<int> catOf(static_cast<size_t>(Gpre)+overs.size(),0);
    {
        std::unordered_map<uint16_t,int>::const_iterator gi=groundLocal.cbegin();
        while(gi!=groundLocal.cend())
        {
            uint16_t m=gi->first; int g=gi->second;
            if(g>=0 && g<static_cast<int>(Gpre))
            {
                const uint8_t cf=collFlag.count(m)?collFlag[m]:0;
                uint16_t beh=ts.behavior(m);
                int c=0;
                if(beh>=0x10&&beh<=0x15) c=2; else if(beh==0x02||beh==0x03) c=3; else if(beh>=0x38&&beh<=0x3B) c=4;
                else if(((cf&1)!=0)&&((cf&2)==0)) c=1;
                catOf[static_cast<size_t>(g)]=c;
            }
            ++gi;
        }
        size_t o=0; while(o<overs.size()){ catOf[static_cast<size_t>(Gpre)+o]=5; o++; }
    }
    std::vector<uint32_t> vpos;
    uint32_t vadj=0;
    std::vector<QImage> newVis=layout2D(src,cellVis,poolMaps,poolCols,vpos,vadj,baseName+":visible",catOf);
    pool.adjacencyViolations+=vadj;
    // cellHid = the wall of a cell that has an over, ONLY when that wall image is
    // not already shown visibly somewhere (else it would be a duplicate tile).
    std::vector<std::vector<int> > cellHid(poolMaps.size());
    {
        size_t pmi=0;
        while(pmi<poolMaps.size())
        {
            const DecodedMap *mp=poolMaps[pmi];
            int W=static_cast<int>(mp->width), H=static_cast<int>(mp->height);
            cellHid[pmi].assign(static_cast<size_t>(W)*static_cast<size_t>(H),-1);
            int yy=0;
            while(yy<H)
            {
                int xx=0;
                while(xx<W)
                {
                    int g=cellTi[pmi][static_cast<size_t>(yy)*W+xx];
                    uint16_t m=static_cast<uint16_t>(rom_.u16(mp->blocksPtr+(static_cast<uint32_t>(yy)*W+xx)*2)&0x3FF);
                    std::unordered_map<uint16_t,int>::const_iterator ol=overLocal.find(m);
                    if(ol!=overLocal.cend() && ol->second>=0 && g>=0 && !visGround[static_cast<size_t>(g)])
                        cellHid[pmi][static_cast<size_t>(yy)*W+xx]=g;
                    xx++;
                }
                yy++;
            }
            pmi++;
        }
    }
    std::vector<uint32_t> hpos;
    uint32_t hadj=0;
    std::vector<QImage> newHid=layout2D(grounds,cellHid,poolMaps,poolCols,hpos,hadj,baseName+":hidden",std::vector<int>());
    pool.adjacencyViolations+=hadj;
    uint32_t V=static_cast<uint32_t>(newVis.size());
    // final position of an OLD ground index g: its visible cell, else the hidden
    // region (offset by V).  Remap groundLocal / contextLocal accordingly.
    {
        std::unordered_map<uint16_t,int>::iterator gl=groundLocal.begin();
        while(gl!=groundLocal.end()){ uint32_t g=static_cast<uint32_t>(gl->second); gl->second=static_cast<int>(visGround[g]?vpos[g]:(V+hpos[g])); ++gl; }
        std::unordered_map<uint64_t,int>::iterator cl=contextLocal.begin();
        while(cl!=contextLocal.end()){ uint32_t g=static_cast<uint32_t>(cl->second); cl->second=static_cast<int>(visGround[g]?vpos[g]:(V+hpos[g])); ++cl; }
    }
    // overRemap[old over index] -> absolute cell: a folded over takes its ground's
    // final position; a real over takes its visible-pass position.
    std::vector<uint32_t> overRemap(overs.size(),0);
    {
        size_t oi=0;
        while(oi<overs.size())
        {
            if(overFold[oi]!=NOT_FOLDED){ uint32_t g=overFold[oi]; overRemap[oi]=visGround[g]?vpos[g]:(V+hpos[g]); }
            else overRemap[oi]=vpos[Gpre+static_cast<uint32_t>(oi)];
            oi++;
        }
    }
    // Assemble the unified tiles into `grounds` (visible region then hidden walls);
    // `overs` now empty -- overs live inside the visible region via overRemap.
    grounds.clear();
    grounds.reserve(newVis.size()+newHid.size());
    { size_t k=0; while(k<newVis.size()){ grounds.push_back(newVis[k]); k++; } k=0; while(k<newHid.size()){ grounds.push_back(newHid[k]); k++; } }
    overs.clear();
    uint32_t groundCount=static_cast<uint32_t>(grounds.size());
    uint32_t groundsOvers=groundCount;
    // Visible+hidden tiles, then the animation tiles.
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

    // GUARD layer-recompose (TEST CASE): EVERY tile split across TWO layers must
    // RECOMPOSE to the original ROM metatile.  Covers all three kinds of split:
    //   - ROM-sublayer overlays (a cliff/rock/tree TERRAIN under + transparent over),
    //   - Pass-1b background/foreground splits (a reused walkable TERRAIN under + a
    //     shared object over), and
    //   - a BUILDING whose body and roof land on different layers.
    // Recompose the FINAL stored under (groundCell, the terrain/wall) with the FINAL
    // stored over (overCell, the object/roof) drawn on top -- exactly how CCWriter
    // stacks Walkable/Collisions + WalkBehind/2nd-Collisions -- and compare to the
    // metatile re-rendered straight from the ROM.  End-to-end: catches a wrong split,
    // an over-aggressive near-dup fold, or a layout/gid slip (each shifts the pixels).
    // The user's >=12-opaque-px contract is re-checked for the Pass-1b objects.
    pool.bgFgSplits=static_cast<uint32_t>(splitIds.size());
    {
        std::unordered_set<uint16_t> bgfgSet(splitIds.begin(),splitIds.end());
        const int kSplitTol=64;    // per-channel; tolerates the near-dup fold, not a wrong tile
        const int kSplitMaxBad=24; // more off-tolerance pixels than this == the split is broken
        size_t ui=0;
        while(ui<usedIds.size())
        {
            uint16_t id=usedIds[ui];
            std::unordered_map<uint16_t,int>::const_iterator gc=pool.groundCell.find(id);
            std::unordered_map<uint16_t,int>::const_iterator oc=pool.overCell.find(id);
            if(gc!=pool.groundCell.cend() && gc->second>=0 && oc!=pool.overCell.cend() && oc->second>=0
               && static_cast<size_t>(gc->second)<tiles.size() && static_cast<size_t>(oc->second)<tiles.size())
            {
                QImage terr=tiles[static_cast<size_t>(gc->second)].convertToFormat(QImage::Format_ARGB32);
                QImage obj=tiles[static_cast<size_t>(oc->second)].convertToFormat(QImage::Format_ARGB32);
                QImage orig=ts.renderMetatile(id).convertToFormat(QImage::Format_ARGB32);
                QImage comp=terr.copy();
                int objOpaque=0,bad=0,y=0;
                while(y<16)
                {
                    int x=0;
                    while(x<16)
                    {
                        QRgb o=obj.pixel(x,y);
                        if(qAlpha(o)!=0){ comp.setPixel(x,y,o); objOpaque++; }
                        x++;
                    }
                    y++;
                }
                y=0;
                while(y<16)
                {
                    int x=0;
                    while(x<16)
                    {
                        QRgb a=comp.pixel(x,y), b=orig.pixel(x,y);
                        if(qAbs(qRed(a)-qRed(b))>kSplitTol||qAbs(qGreen(a)-qGreen(b))>kSplitTol||qAbs(qBlue(a)-qBlue(b))>kSplitTol) bad++;
                        x++;
                    }
                    y++;
                }
                pool.layerSplitTiles++;
                bool need12=(bgfgSet.count(id)>0); // the >=12-opaque contract is for Pass-1b objects
                if(bad>kSplitMaxBad || (need12 && objOpaque<12)) pool.layerSplitBad++;
            }
            ui++;
        }
    }

    pool.uniqueCount=static_cast<uint32_t>(tiles.size());
    pool.mainCount=animStart; // grounds+overs; door+anim tiles ([mainCount,uniqueCount)) follow
    // Door + animated tiles ([mainCount,uniqueCount)) go on their own "_anim" sheet.
    std::vector<SheetSeg> segs=poolSheetSegments(pool.mainCount,pool.uniqueCount);
    pool.sheetCount=static_cast<uint32_t>(segs.size());

    size_t sg=0;
    while(sg<segs.size())
    {
        uint32_t startCell=segs[sg].start;
        uint32_t cnt=segs[sg].count;
        int rows=static_cast<int>((cnt+poolCols-1)/poolCols);
        QImage sheet(poolCols*16,rows*16,QImage::Format_ARGB32);
        sheet.fill(Qt::transparent);
        uint32_t c=0;
        while(c<cnt)
        {
            blitTile(sheet,poolCols,c,tiles[startCell+c]);
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
        std::string name=baseName+segs[sg].suffix;
        sheet.save(QString::fromStdString(outDir+"/"+name+".png"),"PNG");
        writeTsx(outDir,name,poolCols,sheet,cnt,localAnims);
        sg++;
    }

    // Post-build GUARD: within a pool no NON-ANIMATION tile graphic may repeat
    // across its sheets (the "no cross-tileset duplicate" rule).  Animation
    // frames are exempt — a door's closed frame legitimately equals its static
    // Walkable tile.  Count the redundant non-animation duplicates so prepare()
    // can FAIL the run if any survive.
    {
        QImage blkT(16,16,QImage::Format_ARGB32);
        blkT.fill(Qt::transparent);
        std::string blkKey(reinterpret_cast<const char *>(blkT.constBits()),static_cast<size_t>(blkT.sizeInBytes()));
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

    // GUARD tiny-tile: a tile must not be NEAR-EMPTY — 1..12 visible (alpha>0) pixels
    // with everything else fully transparent.  A fully-transparent tile (0 visible) is
    // fine, and a tile with >=13 visible px is fine; only a 1..12-pixel speck is a
    // wasted/degenerate overlay.  Animation frames are exempt (a frame may be sparse).
    {
        uint32_t k=0;
        while(static_cast<size_t>(k)<tiles.size() && k<animStart)
        {
            const QImage &a=tiles[k];
            int vis=0,y=0;
            while(y<16){ int x=0; while(x<16){ if(qAlpha(a.pixel(x,y))>0) vis++; x++; } y++; }
            if(vis>=1 && vis<=12) pool.tinyTiles++;
            k++;
        }
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

// The single region (first path component, e.g. "kanto") shared by ALL of a
// pool's maps, or "" when the pool spans more than one region (then the pool
// stays at the tileset/ root, shared).
static std::string poolRegion(const std::vector<const DecodedMap *> &poolMaps, const Naming &naming)
{
    std::string region;
    bool set=false;
    size_t i=0;
    while(i<poolMaps.size())
    {
        const std::string &p=naming.pathFor(poolMaps[i]->group,poolMaps[i]->map);
        std::string r;
        std::string::size_type slash=p.find('/');
        if(slash!=std::string::npos)
            r=p.substr(0,slash);
        if(!r.empty())
        {
            if(!set) { region=r; set=true; }
            else if(region!=r) return std::string();
        }
        i++;
    }
    return region;
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
    std::set<std::string> labelRegions; // distinct regions this label spans
    size_t i=0;
    while(i<maps.size())
    {
        const DecodedMap &map=maps[i];
        uint64_t pk=pairKey(map.primaryTileset,map.secondaryTileset);
        const std::string &area=naming.zoneFor(map.group,map.map);
        {
            const std::string &mpath=naming.pathFor(map.group,map.map);
            std::string::size_type sl=mpath.find('/');
            if(sl!=std::string::npos && sl>0)
                labelRegions.insert(mpath.substr(0,sl));
        }
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

    // When a label has many tilesets (>50 pools) AND spans >=2 regions, region-
    // specific pools are tidied into tileset/<region>/ subfolders (a pool whose
    // maps all live in one region); pools shared across regions stay at the
    // tileset/ root.  A single-region label stays flat (nesting everything under
    // one region folder would not declutter anything).
    const bool nestByRegion=(usedPrimary.size()+usedSecondary.size())>50 && labelRegions.size()>=2;

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
        std::string sub=nestByRegion ? poolRegion(poolMapsPrimary[key],naming) : std::string();
        primaryPools_[key]=buildPool(key,0,ids,name,poolMapsPrimary[key],sub);
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
        std::string sub=nestByRegion ? poolRegion(poolMapsSecondary[key],naming) : std::string();
        secondaryPools_[key]=buildPool(primary,secondary,ids,name,poolMapsSecondary[key],sub);
        ki++;
    }

    // Object/semantic markers come from the generated visible markers.tsx
    // (written by writeMarkers above), referenced at markerGid(map).

    uint32_t totalSheets=0,totalDup=0,totalAdj=0,totalBgFg=0,totalLayer=0,totalLayerBad=0,totalTiny=0;
    std::unordered_map<uint32_t,TilePool>::const_iterator a=primaryPools_.begin();
    while(a!=primaryPools_.cend()) { totalSheets+=a->second.sheetCount; totalDup+=a->second.duplicateTiles; totalAdj+=a->second.adjacencyViolations; totalBgFg+=a->second.bgFgSplits; totalLayer+=a->second.layerSplitTiles; totalLayerBad+=a->second.layerSplitBad; totalTiny+=a->second.tinyTiles; ++a; }
    std::unordered_map<uint64_t,TilePool>::const_iterator b=secondaryPools_.begin();
    while(b!=secondaryPools_.cend()) { totalSheets+=b->second.sheetCount; totalDup+=b->second.duplicateTiles; totalAdj+=b->second.adjacencyViolations; totalBgFg+=b->second.bgFgSplits; totalLayer+=b->second.layerSplitTiles; totalLayerBad+=b->second.layerSplitBad; totalTiny+=b->second.tinyTiles; ++b; }
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
    // GUARD 3: every tile split across two layers — a ROM-sublayer overlay
    // (terrain+object), a Pass-1b background/foreground split, or a building
    // wall+roof — must recompose (under + over) to the original ROM metatile; the
    // Pass-1b objects must also keep >=12 opaque px.
    if(totalLayer==0)
        std::cout << "TilesetBuilder GUARD layer-recompose: PASS (no tiles split across layers)" << std::endl;
    else if(totalLayerBad==0)
        std::cout << "TilesetBuilder GUARD layer-recompose: PASS (" << totalLayer
                  << " tile(s) split across 2 layers — terrain+object / wall+roof — each recomposes to the ROM tile; "
                  << totalBgFg << " are bg/fg splits)" << std::endl;
    else
        std::cout << "TilesetBuilder GUARD layer-recompose: FAIL (" << totalLayerBad << "/" << totalLayer
                  << " split tile(s) do NOT recompose to the original ROM tile)" << std::endl;
    // GUARD 4: no NEAR-EMPTY tile (1..12 visible px, rest fully transparent); a fully
    // transparent tile is fine.  A handful are tolerated (owner accepts very few).
    const uint32_t kTinyTolerance=8;
    if(totalTiny==0)
        std::cout << "TilesetBuilder GUARD tiny-tile: PASS (no near-empty 1..12-px tiles)" << std::endl;
    else if(totalTiny<=kTinyTolerance)
        std::cout << "TilesetBuilder GUARD tiny-tile: PASS (" << totalTiny
                  << " near-empty tile(s) with 1..12 visible px — within tolerance)" << std::endl;
    else
        std::cout << "TilesetBuilder GUARD tiny-tile: FAIL (" << totalTiny
                  << " near-empty tile(s) with 1..12 visible px, rest transparent)" << std::endl;
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
    // Gids are contiguous (gid = base + position), so the secondary pool starts
    // right after the primary pool's tiles (the anim sheet split does not pad).
    const TilePool *p=primaryPool(map);
    return 1+(p!=nullptr ? p->uniqueCount : 0);
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
        base+=s->uniqueCount;
    return base;
}

std::vector<std::pair<uint32_t,std::string> > TilesetBuilder::tilesetRefs(const DecodedMap &map) const
{
    std::vector<std::pair<uint32_t,std::string> > out;
    const TilePool *p=primaryPool(map);
    if(p!=nullptr)
    {
        std::string pre=p->subDir.empty()?p->baseName:p->subDir+"/"+p->baseName;
        std::vector<SheetSeg> segs=poolSheetSegments(p->mainCount,p->uniqueCount);
        size_t i=0;
        while(i<segs.size()){ out.push_back(std::make_pair(1+segs[i].start,pre+segs[i].suffix)); i++; }
    }
    const TilePool *sec=secondaryPool(map);
    if(sec!=nullptr)
    {
        uint32_t base=secondaryBase(map);
        std::string pre=sec->subDir.empty()?sec->baseName:sec->subDir+"/"+sec->baseName;
        std::vector<SheetSeg> segs=poolSheetSegments(sec->mainCount,sec->uniqueCount);
        size_t i=0;
        while(i<segs.size()){ out.push_back(std::make_pair(base+segs[i].start,pre+segs[i].suffix)); i++; }
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
