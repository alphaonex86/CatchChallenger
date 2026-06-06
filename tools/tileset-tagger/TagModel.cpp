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
    columns_=tileset.attribute("columns","1").toInt();
    if(columns_<1)
        columns_=1;

    // resolve and load the referenced image (relative to the .tsx directory)
    QDomElement image=tileset.firstChildElement("image");
    if(!image.isNull())
    {
        const QString src=image.attribute("source");
        imagePath_=QDir(QFileInfo(tsxPath).absolutePath()).absoluteFilePath(src);
        if(!image_.load(imagePath_))
            image_=QImage(); // guard becomes a no-op rather than failing the whole load
        if(tileCount_<=0 && !image_.isNull() && tileWidth_>0 && tileHeight_>0)
            tileCount_=(image_.width()/tileWidth_)*(image_.height()/tileHeight_);
    }

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
                else if(n=="name")
                    tag.name=v.toStdString();
                else if(n=="size")
                    tag.size=v.toStdString();
                prop=prop.nextSiblingElement("property");
            }
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
                        const std::string &name, const std::string &size)
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
            tag.name=name;
            tag.size=size;
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

bool TagModel::save()
{
    // sync the in-memory tags into the DOM
    std::unordered_map<int,TileTag>::const_iterator it=tags_.begin();
    while(it!=tags_.cend())
    {
        const int id=it->first;
        const TileTag &tag=it->second;
        QDomElement tile=ensureTileElement(id);
        QDomElement props=tile.firstChildElement("properties");
        if(props.isNull())
        {
            props=doc_.createElement("properties");
            tile.insertBefore(props,tile.firstChild());
        }
        // upsert each property
        const char *names[3]={"category","name","size"};
        const std::string values[3]={tag.category,tag.name,tag.size};
        int p=0;
        while(p<3)
        {
            QDomElement prop=props.firstChildElement("property");
            QDomElement found;
            while(!prop.isNull())
            {
                if(prop.attribute("name")==names[p]) { found=prop; break; }
                prop=prop.nextSiblingElement("property");
            }
            if(values[p].empty())
            {
                if(!found.isNull())
                    props.removeChild(found);
            }
            else
            {
                if(found.isNull())
                {
                    found=doc_.createElement("property");
                    found.setAttribute("name",names[p]);
                    props.appendChild(found);
                }
                found.setAttribute("value",QString::fromStdString(values[p]));
            }
            p++;
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
