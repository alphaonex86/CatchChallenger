#include "TagModel.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

TagModel::TagModel() :
    tsxPath_(),
    imagePath_(),
    doc_(),
    image_(),
    tileWidth_(16),
    tileHeight_(16),
    tileCount_(0),
    columns_(1),
    tags_(),
    animatedTiles_(),
    emptyTag_(),
    error_()
{
}

QDomElement TagModel::tilesetElement()
{
    return doc_.documentElement(); // <tileset>
}

bool TagModel::load(const QString &tsxPath)
{
    tsxPath_=tsxPath;
    tags_.clear();
    animatedTiles_.clear();
    QFile file(tsxPath);
    if(!file.open(QIODevice::ReadOnly))
    {
        error_="cannot open "+tsxPath;
        return false;
    }
    QString parseError;
    int line=0,col=0;
    if(!doc_.setContent(&file,&parseError,&line,&col))
    {
        file.close();
        error_="XML parse error in "+tsxPath+": "+parseError+" (line "+QString::number(line)+")";
        return false;
    }
    file.close();

    QDomElement tileset=tilesetElement();
    if(tileset.isNull() || tileset.tagName()!="tileset")
    {
        error_="not a <tileset>: "+tsxPath;
        return false;
    }
    tileWidth_=tileset.attribute("tilewidth","16").toInt();
    tileHeight_=tileset.attribute("tileheight","16").toInt();
    tileCount_=tileset.attribute("tilecount","0").toInt();
    // old minimal CC tilesets omit columns/tilecount; derive them from the image
    // below. 0 here means "unknown, derive".
    columns_=tileset.attribute("columns","0").toInt();

    // resolve and load the referenced image (relative to the .tsx directory)
    QDomElement image=tileset.firstChildElement("image");
    if(!image.isNull())
    {
        const QString src=image.attribute("source");
        imagePath_=QDir(QFileInfo(tsxPath).absolutePath()).absoluteFilePath(src);
        if(!image_.load(imagePath_))
            image_=QImage(); // guard becomes a no-op rather than failing the whole load
        if(columns_<1 && !image_.isNull() && tileWidth_>0)
            columns_=image_.width()/tileWidth_;
        if(tileCount_<=0 && !image_.isNull() && tileWidth_>0 && tileHeight_>0)
            tileCount_=(image_.width()/tileWidth_)*(image_.height()/tileHeight_);
    }
    if(columns_<1)
        columns_=1;

    // read existing category/name/size tags
    QDomElement tile=tileset.firstChildElement("tile");
    while(!tile.isNull())
    {
        const int id=tile.attribute("id","-1").toInt();
        QDomElement props=tile.firstChildElement("properties");
        if(id>=0 && !props.isNull())
        {
            TileTag tag;
            QDomElement prop=props.firstChildElement("property");
            while(!prop.isNull())
            {
                const QString n=prop.attribute("name");
                const QString v=prop.attribute("value");
                if(n=="category")
                    tag.category=v.toStdString();
                else
                    tag.attributes[n.toStdString()]=v.toStdString();
                if(n=="animation" || n=="frames")
                    animatedTiles_.insert(id);   // foreign anim hint -> pre-fill 'animated'
                prop=prop.nextSiblingElement("property");
            }
            // a tile is OURS only if it carries a category; otherwise its
            // properties are foreign (engine/Tiled) and we leave them untouched.
            if(!tag.category.empty())
                tags_[id]=tag;
        }
        tile=tile.nextSiblingElement("tile");
    }
    error_.clear();
    return true;
}

QDomElement TagModel::ensureTileElement(int id)
{
    QDomElement tileset=tilesetElement();
    QDomElement tile=tileset.firstChildElement("tile");
    while(!tile.isNull())
    {
        if(tile.attribute("id","-1").toInt()==id)
            return tile;
        tile=tile.nextSiblingElement("tile");
    }
    QDomElement created=doc_.createElement("tile");
    created.setAttribute("id",id);
    tileset.appendChild(created);
    return created;
}

void TagModel::tagTiles(const std::vector<int> &tileIds, const std::string &category,
                        const std::map<std::string,std::string> &attributes)
{
    size_t i=0;
    while(i<tileIds.size())
    {
        const int id=tileIds.at(i);
        if(category.empty())
            tags_.erase(id);
        else
        {
            TileTag tag;
            tag.category=category;
            tag.attributes=attributes;
            tags_[id]=tag;
        }
        i++;
    }
}

std::vector<int> TagModel::tilesInRect(int col0, int row0, int col1, int row1) const
{
    if(col0>col1) { const int t=col0; col0=col1; col1=t; }
    if(row0>row1) { const int t=row0; row0=row1; row1=t; }
    std::vector<int> ids;
    int row=row0;
    while(row<=row1)
    {
        int colmn=col0;
        while(colmn<=col1)
        {
            const int id=row*columns_+colmn;
            if(colmn>=0 && colmn<columns_ && id>=0 && id<tileCount_)
                ids.push_back(id);
            colmn++;
        }
        row++;
    }
    return ids;
}

const TagModel::TileTag &TagModel::tagOf(int tileId) const
{
    std::unordered_map<int,TileTag>::const_iterator it=tags_.find(tileId);
    return it==tags_.cend() ? emptyTag_ : it->second;
}

bool TagModel::tileHasPixels(int tileId) const
{
    if(image_.isNull() || columns_<1 || tileWidth_<1 || tileHeight_<1)
        return false;
    const int col=tileId%columns_;
    const int row=tileId/columns_;
    const int x0=col*tileWidth_;
    const int y0=row*tileHeight_;
    if(x0+tileWidth_>image_.width() || y0+tileHeight_>image_.height())
        return false;
    if(!image_.hasAlphaChannel())
        return true; // fully opaque sheet: every in-bounds tile draws
    int y=0;
    while(y<tileHeight_)
    {
        int x=0;
        while(x<tileWidth_)
        {
            if(qAlpha(image_.pixel(x0+x,y0+y))!=0)
                return true;
            x++;
        }
        y++;
    }
    return false;
}

bool TagModel::tileAnimated(int tileId) const
{
    return animatedTiles_.find(tileId)!=animatedTiles_.cend();
}

std::vector<int> TagModel::untaggedNonEmpty() const
{
    std::vector<int> out;
    int id=0;
    while(id<tileCount_)
    {
        if(tags_.find(id)==tags_.cend() && tileHasPixels(id))
            out.push_back(id);
        id++;
    }
    return out;
}

std::vector<std::string> TagModel::categoriesUsed() const
{
    std::vector<std::string> cats;
    std::unordered_map<int,TileTag>::const_iterator it=tags_.begin();
    while(it!=tags_.cend())
    {
        const std::string &c=it->second.category;
        bool seen=false;
        size_t k=0;
        while(k<cats.size()) { if(cats.at(k)==c) { seen=true; break; } k++; }
        if(!seen)
            cats.push_back(c);
        ++it;
    }
    return cats;
}

static bool isKnownTagKey(const QString &n)
{
    // the attribute vocabulary this tool owns; foreign properties are NOT here
    // and are preserved across save.  (legacy "name" kept so old tags get cleaned)
    return n=="category" || n=="group" || n=="name" || n=="size"
        || n=="layer" || n=="walkable" || n=="animated"
        || n=="horizontalRepeat" || n=="horizontalMiddleRepeat"
        || n=="verticalRepeat" || n=="verticalMiddleRepeat"
        || n=="terrain";
}

void TagModel::stripManagedProperties(QDomElement props, const std::map<std::string,std::string> *extraKeys)
{
    if(props.isNull())
        return;
    std::vector<QDomElement> doomed;
    QDomElement prop=props.firstChildElement("property");
    while(!prop.isNull())
    {
        const QString n=prop.attribute("name");
        bool managed=isKnownTagKey(n);
        if(!managed && extraKeys!=nullptr && extraKeys->find(n.toStdString())!=extraKeys->cend())
            managed=true;
        if(managed)
            doomed.push_back(prop);
        prop=prop.nextSiblingElement("property");
    }
    size_t i=0;
    while(i<doomed.size()) { props.removeChild(doomed.at(i)); i++; }
}

bool TagModel::save()
{
    QDomElement tileset=tilesetElement();

    // 1. clean stale tag properties off tiles that are no longer tagged (so
    //    un-tagging in the GUI actually removes the category from the file).
    QDomElement tile=tileset.firstChildElement("tile");
    while(!tile.isNull())
    {
        const int id=tile.attribute("id","-1").toInt();
        if(id>=0 && tags_.find(id)==tags_.cend())
        {
            QDomElement props=tile.firstChildElement("properties");
            stripManagedProperties(props,nullptr);
            if(!props.isNull() && props.firstChildElement("property").isNull())
                tile.removeChild(props); // drop the now-empty <properties>
        }
        tile=tile.nextSiblingElement("tile");
    }

    // 2. write the tagged tiles: category first, then each attribute.
    std::unordered_map<int,TileTag>::const_iterator it=tags_.begin();
    while(it!=tags_.cend())
    {
        const int id=it->first;
        const TileTag &tag=it->second;
        QDomElement t=ensureTileElement(id);
        QDomElement props=t.firstChildElement("properties");
        if(props.isNull())
        {
            props=doc_.createElement("properties");
            t.insertBefore(props,t.firstChild());
        }
        stripManagedProperties(props,&tag.attributes);
        QDomElement cat=doc_.createElement("property");
        cat.setAttribute("name","category");
        cat.setAttribute("value",QString::fromStdString(tag.category));
        props.appendChild(cat);
        std::map<std::string,std::string>::const_iterator a=tag.attributes.begin();
        while(a!=tag.attributes.cend())
        {
            if(!a->second.empty())
            {
                QDomElement p=doc_.createElement("property");
                p.setAttribute("name",QString::fromStdString(a->first));
                p.setAttribute("value",QString::fromStdString(a->second));
                props.appendChild(p);
            }
            ++a;
        }
        ++it;
    }

    QFile out(tsxPath_);
    if(!out.open(QIODevice::WriteOnly|QIODevice::Truncate))
    {
        error_="cannot write "+tsxPath_;
        return false;
    }
    QTextStream ts(&out);
    doc_.save(ts,1);
    out.close();
    error_.clear();
    return true;
}

int TagModel::tileCount() const { return tileCount_; }
int TagModel::columns() const { return columns_; }
int TagModel::tileWidth() const { return tileWidth_; }
int TagModel::tileHeight() const { return tileHeight_; }
const QImage &TagModel::image() const { return image_; }
const QString &TagModel::error() const { return error_; }
