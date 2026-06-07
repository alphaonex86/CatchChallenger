#include "MapUsageIndex.hpp"

#include "grouplayer.h"
#include "layer.h"
#include "map.h"
#include "mapreader.h"
#include "orthogonalrenderer.h"
#include "tile.h"
#include "tilelayer.h"
#include "tileset.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <set>

MapUsageIndex::MapUsageIndex() :
    tsxCanonical_(),
    candidates_(),
    loaded_(),
    rendered_(),
    lastStats_(),
    error_()
{
    lastStats_.totalCells=0;
    lastStats_.horizontalRepeatCells=0;
    lastStats_.verticalRepeatCells=0;
    lastStats_.walkableCells=0;
    lastStats_.blockedCells=0;
    lastStats_.ledgeCells=0;
}

// A structural layer has a tile at (x,y)?  Mirrors the engine reading the
// Walkable/Collisions/... binary (a non-empty cell == the byte is non-zero).
static bool cellHasTile(Tiled::TileLayer *layer,int x,int y)
{
    if(layer==nullptr)
        return false;
    if(x<0 || y<0 || x>=layer->width() || y>=layer->height())
        return false;
    return !layer->cellAt(x,y).isEmpty();
}

static bool anyHasTile(const std::vector<Tiled::TileLayer*> &layers,int x,int y)
{
    size_t i=0;
    while(i<layers.size()) { if(cellHasTile(layers.at(i),x,y)) return true; i++; }
    return false;
}

MapUsageIndex::~MapUsageIndex()
{
}

// Recurse into group layers so nested tile layers are found too.
static void collectTileLayers(const QList<Tiled::Layer*> &layers,std::vector<Tiled::TileLayer*> &out)
{
    int i=0;
    while(i<layers.size())
    {
        Tiled::Layer *l=layers.at(i);
        if(l->isTileLayer())
            out.push_back(static_cast<Tiled::TileLayer*>(l));
        else if(l->isGroupLayer())
            collectTileLayers(static_cast<Tiled::GroupLayer*>(l)->layers(),out);
        i++;
    }
}

void MapUsageIndex::build(const QString &tsxPath)
{
    tsxCanonical_=QFileInfo(tsxPath).canonicalFilePath();
    candidates_.clear();
    loaded_.clear();
    rendered_.clear();
    error_.clear();

    // Scope of the usage scan: a tileset inside a "tileset/" dir is used only by the
    // maps in its SIBLING tree, so scan the PARENT of "tileset/".  A per-label ROM
    // tileset map/main/firered/tileset/ -> scan firered/ (fast, and correct: only
    // firered maps use it); the shared official map/tileset/ -> scan map/ (all maps).
    // Falls back to the nearest ancestor named "map".
    QDir dir=QFileInfo(tsxPath).absoluteDir();
    QString root=dir.absolutePath();
    if(dir.dirName()=="tileset")
    {
        QDir gp=dir;
        if(gp.cdUp())
            root=gp.absolutePath();
    }
    else
    {
        QDir walk=dir;
        bool found=false;
        while(!found && walk.cdUp())
        {
            if(walk.dirName()=="map")
            {
                root=walk.absolutePath();
                found=true;
            }
        }
    }

    const QString base=QFileInfo(tsxPath).fileName();   // e.g. "general-0.tsx"
    QDirIterator it(root,QStringList()<<"*.tmx",QDir::Files,QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        const QString tmx=it.next();
        QFile f(tmx);
        if(f.open(QIODevice::ReadOnly))
        {
            const QByteArray content=f.readAll();
            f.close();
            // cheap pre-filter: the .tmx must mention the .tsx by basename.
            if(content.contains(base.toUtf8()))
                candidates_.push_back(tmx);
        }
    }
}

int MapUsageIndex::candidateCount() const
{
    return (int)candidates_.size();
}

Tiled::Map *MapUsageIndex::mapFor(const QString &path)
{
    std::map<QString,std::shared_ptr<Tiled::Map> >::iterator it=loaded_.find(path);
    if(it!=loaded_.end())
        return it->second.get();
    Tiled::MapReader reader;
    std::shared_ptr<Tiled::Map> m=std::shared_ptr<Tiled::Map>(reader.readMap(path).release());
    loaded_[path]=m;     // cache even null (avoids re-reading a broken map)
    return m.get();
}

std::vector<MapUsageIndex::Usage> MapUsageIndex::findUsages(const std::vector<int> &tileIds)
{
    std::set<int> ids(tileIds.begin(),tileIds.end());
    std::vector<Usage> out;
    lastStats_.layerCounts.clear();
    lastStats_.totalCells=0;
    lastStats_.horizontalRepeatCells=0;
    lastStats_.verticalRepeatCells=0;
    lastStats_.walkableCells=0;
    lastStats_.blockedCells=0;
    lastStats_.ledgeCells=0;
    if(ids.empty())
        return out;

    size_t ci=0;
    while(ci<candidates_.size())
    {
        const QString path=candidates_.at(ci);
        Tiled::Map *m=mapFor(path);
        if(m!=nullptr)
        {
            // find the loaded tileset that IS our .tsx (by canonical path)
            Tiled::Tileset *target=nullptr;
            const QVector<Tiled::SharedTileset> &tss=m->tilesets();
            int t=0;
            while(t<tss.size())
            {
                const QString fn=QFileInfo(tss.at(t)->fileName()).canonicalFilePath();
                if(!fn.isEmpty() && fn==tsxCanonical_)
                    target=tss.at(t).data();
                t++;
            }
            if(target!=nullptr)
            {
                Usage u;
                u.mapPath=path;
                u.mapLabel=QFileInfo(path).fileName();
                u.mapW=m->width();
                u.mapH=m->height();
                u.tileW=m->tileWidth();
                u.tileH=m->tileHeight();
                std::vector<Tiled::TileLayer*> layers;
                collectTileLayers(m->layers(),layers);
                // gather the structural layers ONCE so we can compute each usage
                // cell's EFFECTIVE walkability with the engine precedence below.
                Tiled::TileLayer *Lwalk=nullptr,*Lcoll=nullptr,*Ldirt=nullptr;
                Tiled::TileLayer *Lll=nullptr,*Llr=nullptr,*Llt=nullptr,*Llb=nullptr;
                std::vector<Tiled::TileLayer*> monsterLayers;
                size_t sl=0;
                while(sl<layers.size())
                {
                    Tiled::TileLayer *L=layers.at(sl);
                    const QString n=L->name();
                    if(n=="Walkable") Lwalk=L;
                    else if(n=="Collisions") Lcoll=L;
                    else if(n=="Dirt") Ldirt=L;
                    else if(n=="LedgesLeft") Lll=L;
                    else if(n=="LedgesRight") Llr=L;
                    else if(n=="LedgesTop" || n=="LedgesUp") Llt=L;
                    else if(n=="LedgesBottom" || n=="LedgesDown") Llb=L;
                    else if(n=="Grass" || n=="OnGrass" || n=="Water" || n=="Lava") monsterLayers.push_back(L);
                    sl++;
                }
                size_t li=0;
                while(li<layers.size())
                {
                    Tiled::TileLayer *tl=layers.at(li);
                    const int w=tl->width();
                    const int h=tl->height();
                    const std::string layerName=tl->name().toStdString();
                    int y=0;
                    while(y<h)
                    {
                        int x=0;
                        while(x<w)
                        {
                            const Tiled::Cell &c=tl->cellAt(x,y);
                            if(!c.isEmpty() && c.tileset()==target && ids.find(c.tileId())!=ids.end())
                            {
                                u.cells.push_back(QPoint(tl->x()+x,tl->y()+y));
                                // stats for pre-fill: PLACEMENT layer + same-tile runs
                                lastStats_.totalCells++;
                                lastStats_.layerCounts[layerName]++;
                                // EFFECTIVE walkability at this cell, engine precedence
                                // (Map_loaderMain.cpp): Dirt/Ledges first, then
                                // Collisions cancels Walkable|zone, else blocked.
                                const int gx=tl->x()+x;
                                const int gy=tl->y()+y;
                                if(cellHasTile(Ldirt,gx,gy) || cellHasTile(Lll,gx,gy) ||
                                   cellHasTile(Llr,gx,gy) || cellHasTile(Llt,gx,gy) || cellHasTile(Llb,gx,gy))
                                    lastStats_.ledgeCells++;
                                else
                                {
                                    bool monster=false;
                                    size_t mi=0;
                                    while(mi<monsterLayers.size() && !monster)
                                    {
                                        if(cellHasTile(monsterLayers.at(mi),gx,gy)) monster=true;
                                        mi++;
                                    }
                                    if((cellHasTile(Lwalk,gx,gy) || monster) && !cellHasTile(Lcoll,gx,gy))
                                        lastStats_.walkableCells++;
                                    else
                                        lastStats_.blockedCells++;
                                }
                                if(x+1<w)
                                {
                                    const Tiled::Cell &rc=tl->cellAt(x+1,y);
                                    if(!rc.isEmpty() && rc.tileset()==target && rc.tileId()==c.tileId())
                                        lastStats_.horizontalRepeatCells++;
                                }
                                if(y+1<h)
                                {
                                    const Tiled::Cell &dc=tl->cellAt(x,y+1);
                                    if(!dc.isEmpty() && dc.tileset()==target && dc.tileId()==c.tileId())
                                        lastStats_.verticalRepeatCells++;
                                }
                            }
                            x++;
                        }
                        y++;
                    }
                    li++;
                }
                if(!u.cells.empty())
                    out.push_back(u);
            }
        }
        ci++;
    }

    // sort by descending usage count (simple insertion: lists are short)
    size_t i=1;
    while(i<out.size())
    {
        Usage key=out.at(i);
        size_t j=i;
        while(j>0 && out.at(j-1).cells.size()<key.cells.size())
        {
            out[j]=out.at(j-1);
            j--;
        }
        out[j]=key;
        i++;
    }
    return out;
}

QImage MapUsageIndex::render(const QString &mapPath)
{
    std::map<QString,QImage>::iterator it=rendered_.find(mapPath);
    if(it!=rendered_.end())
        return it->second;
    Tiled::Map *m=mapFor(mapPath);
    if(m==nullptr)
        return QImage();
    const int tw=m->tileWidth();
    const int th=m->tileHeight();
    QImage img(m->width()*tw,m->height()*th,QImage::Format_ARGB32_Premultiplied);
    img.fill(QColor(28,28,38));
    QPainter p(&img);
    // Render with libtiled's OWN OrthogonalRenderer (exactly what Tiled draws):
    // it applies per-cell FLIP flags (H/V/anti-diagonal), tile draw-offsets and
    // tile sizes — the manual drawPixmap loop ignored all of that, so flipped
    // tiles (very common in these maps: mirrored trees/cliffs/roofs) came out
    // wrong.  Composite every visible tile layer bottom-to-top.
    Tiled::OrthogonalRenderer renderer(m);
    std::vector<Tiled::TileLayer*> layers;
    collectTileLayers(m->layers(),layers);
    size_t li=0;
    while(li<layers.size())
    {
        Tiled::TileLayer *tl=layers.at(li);
        if(tl->isVisible())
            renderer.drawTileLayer(&p,tl);
        li++;
    }
    p.end();
    rendered_[mapPath]=img;
    return img;
}

std::map<int,MapUsageIndex::TileStat> MapUsageIndex::analyzeAllTiles()
{
    std::map<int,TileStat> out;
    size_t ci=0;
    while(ci<candidates_.size())
    {
        Tiled::Map *m=mapFor(candidates_.at(ci));
        if(m!=nullptr)
        {
            Tiled::Tileset *target=nullptr;
            const QVector<Tiled::SharedTileset> &tss=m->tilesets();
            int t=0;
            while(t<tss.size())
            {
                const QString fn=QFileInfo(tss.at(t)->fileName()).canonicalFilePath();
                if(!fn.isEmpty() && fn==tsxCanonical_)
                    target=tss.at(t).data();
                t++;
            }
            if(target!=nullptr)
            {
                std::vector<Tiled::TileLayer*> layers;
                collectTileLayers(m->layers(),layers);
                Tiled::TileLayer *Lwalk=nullptr,*Lcoll=nullptr,*Ldirt=nullptr;
                Tiled::TileLayer *Lll=nullptr,*Llr=nullptr,*Llt=nullptr,*Llb=nullptr;
                std::vector<Tiled::TileLayer*> monsterLayers;
                std::vector<Tiled::TileLayer*> overLayers;
                size_t sl=0;
                while(sl<layers.size())
                {
                    Tiled::TileLayer *L=layers.at(sl);
                    const QString n=L->name();
                    if(n=="Walkable") Lwalk=L;
                    else if(n=="Collisions") Lcoll=L;
                    else if(n=="Dirt") Ldirt=L;
                    else if(n=="LedgesLeft") Lll=L;
                    else if(n=="LedgesRight") Llr=L;
                    else if(n=="LedgesTop" || n=="LedgesUp") Llt=L;
                    else if(n=="LedgesBottom" || n=="LedgesDown") Llb=L;
                    else if(n=="Grass" || n=="OnGrass" || n=="Water" || n=="Lava") monsterLayers.push_back(L);
                    else if(n=="WalkBehind" || n=="Over" || n=="Back") overLayers.push_back(L);
                    sl++;
                }

                // object blob = connected component of (Collisions OR over): small
                // ⇒ tree/rock, large ⇒ building/cliff. One flood-fill per map.
                const int mw=m->width();
                const int mh=m->height();
                std::vector<int> compSize(mw*mh,0);   // 0 = not-object, >0 = blob size
                std::vector<char> compHasOver(mw*mh,0);   // 1 = this blob contains a roof/over tile
                {
                    std::vector<char> obj(mw*mh,0);
                    int yy=0;
                    while(yy<mh)
                    {
                        int xx=0;
                        while(xx<mw)
                        {
                            if(cellHasTile(Lcoll,xx,yy) || anyHasTile(overLayers,xx,yy))
                                obj[xx+yy*mw]=1;
                            xx++;
                        }
                        yy++;
                    }
                    std::vector<int> stack;
                    int idx=0;
                    while(idx<mw*mh)
                    {
                        if(obj[idx]!=0 && compSize[idx]==0)
                        {
                            stack.clear();
                            std::vector<int> cells;
                            stack.push_back(idx);
                            compSize[idx]=-1;
                            while(!stack.empty())
                            {
                                const int p=stack.back();
                                stack.pop_back();
                                cells.push_back(p);
                                const int px=p%mw,py=p/mw;
                                const int nb[4]={px>0?p-1:-1, px<mw-1?p+1:-1, py>0?p-mw:-1, py<mh-1?p+mw:-1};
                                int k=0;
                                while(k<4)
                                {
                                    const int q=nb[k];
                                    if(q>=0 && obj[q]!=0 && compSize[q]==0) { compSize[q]=-1; stack.push_back(q); }
                                    k++;
                                }
                            }
                            const int sz=(int)cells.size();
                            char hasOver=0;
                            size_t co=0;
                            while(co<cells.size()) { const int cc=cells.at(co); if(anyHasTile(overLayers,cc%mw,cc/mw)) { hasOver=1; break; } co++; }
                            size_t ck=0;
                            while(ck<cells.size()) { compSize[cells.at(ck)]=sz; compHasOver[cells.at(ck)]=hasOver; ck++; }
                        }
                        idx++;
                    }
                }

                size_t li=0;
                while(li<layers.size())
                {
                    Tiled::TileLayer *tl=layers.at(li);
                    const int w=tl->width();
                    const int h=tl->height();
                    const std::string layerName=tl->name().toStdString();
                    int y=0;
                    while(y<h)
                    {
                        int x=0;
                        while(x<w)
                        {
                            const Tiled::Cell &c=tl->cellAt(x,y);
                            if(!c.isEmpty() && c.tileset()==target)
                            {
                                TileStat &s=out[c.tileId()];
                                s.total++;
                                s.layerCounts[layerName]++;
                                const int gx=tl->x()+x;
                                const int gy=tl->y()+y;
                                if(cellHasTile(Ldirt,gx,gy) || cellHasTile(Lll,gx,gy) ||
                                   cellHasTile(Llr,gx,gy) || cellHasTile(Llt,gx,gy) || cellHasTile(Llb,gx,gy))
                                    s.ledgeCells++;
                                else
                                {
                                    bool monster=false;
                                    size_t mi=0;
                                    while(mi<monsterLayers.size() && !monster)
                                    {
                                        if(cellHasTile(monsterLayers.at(mi),gx,gy)) monster=true;
                                        mi++;
                                    }
                                    if((cellHasTile(Lwalk,gx,gy) || monster) && !cellHasTile(Lcoll,gx,gy))
                                        s.walkableCells++;
                                    else
                                        s.blockedCells++;
                                }
                                // structure signals (vertical pairing + blob size)
                                if(anyHasTile(overLayers,gx,gy-1))
                                    s.overAbove++;
                                if(cellHasTile(Lcoll,gx,gy+1))
                                    s.solidBelow++;
                                if(gx>=0 && gy>=0 && gx<mw && gy<mh && compSize[gx+gy*mw]>0)
                                {
                                    s.blobSizeSum+=compSize[gx+gy*mw];
                                    s.blobCount++;
                                    if(compHasOver[gx+gy*mw]) s.blobWithOver++;
                                }
                            }
                            x++;
                        }
                        y++;
                    }
                    li++;
                }
            }
        }
        ci++;
    }
    return out;
}

std::string MapUsageIndex::suggestCategory(const TileStat &s,bool greenish,
                                           std::string &normLayer,bool &walkable,bool &highConfidence)
{
    // dominant drawn-on layer
    QString dom;
    int best=-1;
    std::map<std::string,int>::const_iterator l=s.layerCounts.begin();
    while(l!=s.layerCounts.cend()) { if(l->second>best) { best=l->second; dom=QString::fromStdString(l->first); } ++l; }
    const QString L=dom.toLower();
    walkable=(s.walkableCells+s.ledgeCells)>=s.blockedCells;
    highConfidence=true;

    // unambiguous terrain: layer == visual (high confidence)
    if(L.contains("water")) { normLayer="water"; return "water"; }
    if(L.contains("lava")) { normLayer="lava"; return "lava"; }
    if(L.contains("ledge"))
    {
        normLayer="ledge";
        if(L.contains("down")) return "ledge-down";
        if(L.contains("up")) return "ledge-up";
        if(L.contains("left")) return "ledge-left";
        if(L.contains("right")) return "ledge-right";
        return "ledge-down";
    }
    if(L=="grass") { normLayer="grass"; return "grass-tall"; }

    // ambiguous: best guess from layer + colour + STRUCTURE — mark for review.
    highConfidence=false;
    const double avgBlob = s.blobCount>0 ? (double)s.blobSizeSum/(double)s.blobCount : 0.0;
    // 3-tier blob size: tiny ⇒ rock, medium ⇒ a discrete building, huge sprawling ⇒ cliff/mountain.
    const bool tinyBlob  = avgBlob>0.0 && avgBlob<=4.0;
    const bool hugeBlob  = avgBlob>40.0;
    // positive pairing signal: a roof directly above (catches the top wall row),
    // or this is the top of a structure (solid below) — both ⇒ building part.
    const bool roofAbove = s.total>0 && s.overAbove*2>=s.total;

    if(L=="ongrass") { normLayer="grass"; return greenish ? "bush" : "flower"; }

    // over-tile (drawn above the player): green ⇒ canopy, else ⇒ roof
    if(L.contains("walkbehind") || L=="over" || L=="back")
    {
        normLayer="over";
        return greenish ? "tree-canopy" : "building-roof";
    }

    // collidable
    if(!walkable)
    {
        normLayer="collision";
        // a blob that CONTAINS a roof/over tile is a building mass, not terrain — this
        // is what separates building-small/town walls (huge but roofed) from a real cliff.
        const bool roofedBlob = s.total>0 && s.blobWithOver*2>=s.total;
        if(greenish)
            return tinyBlob ? "bush" : "tree-trunk";        // vegetation
        if(roofAbove || roofedBlob)
            return "building-wall";                          // capped/roofed ⇒ building
        if(tinyBlob)
            return "rock";                                   // small isolated ⇒ rock/sign
        if(hugeBlob)
            return "cliff";                                  // large sprawling, NO roof ⇒ cliff/mountain
        return "building-wall";                              // medium discrete ⇒ building
    }

    if(L=="dirt") { normLayer="walkable"; return "path"; }
    normLayer="walkable";
    return greenish ? "grass-short" : "ground";
}

const MapUsageIndex::GroupStats &MapUsageIndex::lastStats() const { return lastStats_; }

const QString &MapUsageIndex::error() const { return error_; }
