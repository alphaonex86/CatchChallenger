#include "MapDecoder.hpp"
#include "TagModel.hpp"

#include "grouplayer.h"
#include "layer.h"
#include "map.h"
#include "mapreader.h"
#include "orthogonalrenderer.h"
#include "tile.h"
#include "tilelayer.h"
#include "tileset.h"

#include <QColor>
#include <QFileInfo>
#include <QPainter>
#include <map>
#include <memory>

// SEMANTIC colours: the category map should look like the real map when the tags
// are right (grass green, water blue, building gray/red, path brown) so mis-tags
// stand out. Unknown categories fall back to a stable hash colour.
QColor MapDecoder::categoryColor(const std::string &cat)
{
    if(cat.empty())
        return QColor(40,40,48);
    if(cat=="water") return QColor(64,120,220);
    if(cat=="water-edge") return QColor(110,160,230);
    if(cat=="waterfall") return QColor(150,190,240);
    if(cat=="lava") return QColor(230,90,30);
    if(cat=="grass-tall") return QColor(40,150,50);
    if(cat=="grass-short") return QColor(95,195,95);
    if(cat=="bush") return QColor(50,130,40);
    if(cat=="ground") return QColor(200,195,150);
    if(cat=="path") return QColor(180,150,100);
    if(cat=="sand") return QColor(228,215,160);
    if(cat=="tree-trunk") return QColor(110,80,50);
    if(cat=="tree-canopy") return QColor(28,105,38);
    if(cat=="building-wall") return QColor(150,150,160);
    if(cat=="building-roof") return QColor(185,80,70);
    if(cat=="door") return QColor(90,60,40);
    if(cat=="window") return QColor(120,180,210);
    if(cat=="stairs") return QColor(165,145,120);
    if(cat=="sign") return QColor(140,100,60);
    if(cat=="fence") return QColor(165,130,90);
    if(cat=="cliff") return QColor(115,100,92);
    if(cat=="rock") return QColor(135,128,120);
    if(cat=="flower") return QColor(232,120,180);
    if(cat.rfind("ledge",0)==0) return QColor(225,150,50);
    if(cat.rfind("interior",0)==0) return QColor(175,160,200);
    unsigned int h=2166136261u;
    size_t i=0;
    while(i<cat.size()) { h=(h^(unsigned char)cat[i])*16777619u; i++; }
    return QColor::fromHsv((int)(h%360),150,200);
}

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

bool MapDecoder::decode(const QString &mapPath, Result &out, QString &error)
{
    Tiled::MapReader reader;
    std::unique_ptr<Tiled::Map> m=reader.readMap(mapPath);
    if(!m)
    {
        error=reader.errorString();
        return false;
    }
    out.w=m->width();
    out.h=m->height();
    out.tileW=m->tileWidth();
    out.tileH=m->tileHeight();
    out.totalDrawn=0;
    out.tagged=0;
    out.untagged=0;
    out.categoryGrid.assign(out.w*out.h,std::string());
    out.topTilesetCanon.assign(out.w*out.h,std::string());
    out.topTileId.assign(out.w*out.h,-1);

    // one TagModel per tileset (loads that tileset's sidecar tags)
    std::vector<TagModel*> owned;
    std::map<Tiled::Tileset*,TagModel*> tags;
    const QVector<Tiled::SharedTileset> &tss=m->tilesets();
    int t=0;
    while(t<tss.size())
    {
        TagModel *tm=new TagModel();
        if(tm->load(tss.at(t)->fileName()))
        {
            tags[tss.at(t).data()]=tm;
            owned.push_back(tm);
        }
        else
            delete tm;
        t++;
    }

    std::vector<Tiled::TileLayer*> layers;
    collectTileLayers(m->layers(),layers);

    // PER-LAYER (z) category grids — the (x,y,z) tensor (bottom layer = z 0).
    {
        size_t L=0;
        while(L<layers.size())
        {
            Tiled::TileLayer *tl=layers.at(L);
            out.layerNames.push_back(tl->name().toStdString());
            std::vector<std::string> grid(out.w*out.h);
            int y=0;
            while(y<out.h)
            {
                int x=0;
                while(x<out.w)
                {
                    const int lx=x-tl->x();
                    const int ly=y-tl->y();
                    if(lx>=0 && ly>=0 && lx<tl->width() && ly<tl->height())
                    {
                        const Tiled::Cell &c=tl->cellAt(lx,ly);
                        if(!c.isEmpty())
                        {
                            std::map<Tiled::Tileset*,TagModel*>::iterator it=tags.find(c.tileset());
                            if(it!=tags.end())
                                grid[x+y*out.w]=it->second->tagOf(c.tileId()).category;
                        }
                    }
                    x++;
                }
                y++;
            }
            out.layerGrids.push_back(grid);
            L++;
        }
    }

    const int tw=out.tileW;
    const int th=out.tileH;
    out.realRender=QImage(out.w*tw,out.h*th,QImage::Format_ARGB32_Premultiplied);
    out.realRender.fill(QColor(28,28,38));
    out.categoryRender=QImage(out.w*tw,out.h*th,QImage::Format_ARGB32_Premultiplied);
    out.categoryRender.fill(QColor(28,28,38));

    // real render: composite all visible tile layers bottom-to-top, using libtiled's
    // OWN renderer (handles per-cell FLIP flags + the single-image tileset sub-rect +
    // tile offsets — the manual drawPixmap loop got all of that wrong).
    {
        QPainter rp(&out.realRender);
        Tiled::OrthogonalRenderer renderer(m.get());
        size_t li=0;
        while(li<layers.size())
        {
            Tiled::TileLayer *tl=layers.at(li);
            if(tl->isVisible())
                renderer.drawTileLayer(&rp,tl);
            li++;
        }
    }

    // category render + grid: the TOPMOST non-empty tile decides each cell
    {
        QPainter cp(&out.categoryRender);
        int y=0;
        while(y<out.h)
        {
            int x=0;
            while(x<out.w)
            {
                bool hadTile=false;
                std::string cat;
                int li=(int)layers.size()-1;
                while(li>=0)
                {
                    Tiled::TileLayer *tl=layers.at(li);
                    const int lx=x-tl->x();
                    const int ly=y-tl->y();
                    if(lx>=0 && ly>=0 && lx<tl->width() && ly<tl->height())
                    {
                        const Tiled::Cell &c=tl->cellAt(lx,ly);
                        if(!c.isEmpty())
                        {
                            hadTile=true;
                            if(c.tileset()!=nullptr)
                            {
                                out.topTilesetCanon[x+y*out.w]=QFileInfo(c.tileset()->fileName()).canonicalFilePath().toStdString();
                                out.topTileId[x+y*out.w]=c.tileId();
                            }
                            std::map<Tiled::Tileset*,TagModel*>::iterator it=tags.find(c.tileset());
                            if(it!=tags.end())
                                cat=it->second->tagOf(c.tileId()).category;
                            break;   // topmost non-empty tile wins
                        }
                    }
                    li--;
                }
                if(hadTile)
                {
                    out.totalDrawn++;
                    const QRect cell(x*tw,y*th,tw,th);
                    if(!cat.empty())
                    {
                        out.tagged++;
                        out.categoryGrid[x+y*out.w]=cat;
                        cp.fillRect(cell,categoryColor(cat));
                    }
                    else
                    {
                        out.untagged++;
                        cp.fillRect(cell,QColor(255,0,255));   // magenta = a tile with no tag
                    }
                }
                x++;
            }
            y++;
        }
    }

    size_t i=0;
    while(i<owned.size()) { delete owned.at(i); i++; }
    error.clear();
    return true;
}
