#include "MapUsageIndex.hpp"

#include "grouplayer.h"
#include "layer.h"
#include "map.h"
#include "mapreader.h"
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
    error_()
{
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

    // map root = the ancestor directory named "map" (the .tsx lives under
    // map/.../tileset/), else the .tsx's own directory.
    QDir dir=QFileInfo(tsxPath).absoluteDir();
    QString root=dir.absolutePath();
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
                size_t li=0;
                while(li<layers.size())
                {
                    Tiled::TileLayer *tl=layers.at(li);
                    const int w=tl->width();
                    const int h=tl->height();
                    int y=0;
                    while(y<h)
                    {
                        int x=0;
                        while(x<w)
                        {
                            const Tiled::Cell &c=tl->cellAt(x,y);
                            if(!c.isEmpty() && c.tileset()==target && ids.find(c.tileId())!=ids.end())
                                u.cells.push_back(QPoint(tl->x()+x,tl->y()+y));
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
    std::vector<Tiled::TileLayer*> layers;
    collectTileLayers(m->layers(),layers);
    size_t li=0;
    while(li<layers.size())
    {
        Tiled::TileLayer *tl=layers.at(li);
        if(tl->isVisible())
        {
            const int w=tl->width();
            const int h=tl->height();
            int y=0;
            while(y<h)
            {
                int x=0;
                while(x<w)
                {
                    const Tiled::Cell &c=tl->cellAt(x,y);
                    Tiled::Tile *tile=c.tile();
                    if(tile!=nullptr)
                    {
                        const QPixmap &pm=tile->image();
                        if(!pm.isNull())
                            p.drawPixmap((tl->x()+x)*tw,(tl->y()+y)*th,pm);
                    }
                    x++;
                }
                y++;
            }
        }
        li++;
    }
    p.end();
    rendered_[mapPath]=img;
    return img;
}

const QString &MapUsageIndex::error() const { return error_; }
