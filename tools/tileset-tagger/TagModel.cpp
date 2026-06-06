#include "TagModel.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStandardPaths>
#include <cstdlib>

// The datapack stays the READ-ONLY source of truth: tags are work data and live
// OUT of the datapack, under ~/.local/share/catchchallenger/datapack-<sha256(abs
// datapack root path)>/ (per root CLAUDE.md). One tag file per tileset.

// datapack root = the parent of the ancestor "map" dir; else the .tsx's own dir.
static QString datapackRootOf(const QString &tsxPath)
{
    QDir walk=QFileInfo(tsxPath).absoluteDir();
    while(walk.cdUp())
    {
        if(walk.dirName()=="map")
            return QFileInfo(walk.absolutePath()).absolutePath();
    }
    return QFileInfo(tsxPath).absolutePath();
}

static QString sha256Hex(const QString &s)
{
    return QString::fromLatin1(QCryptographicHash::hash(s.toUtf8(),QCryptographicHash::Sha256).toHex());
}

TagModel::TagModel() :
    tsxPath_(),
    imagePath_(),
    sidecarDir_(),
    tagFilePath_(),
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

void TagModel::loadSidecarTags()
{
    tags_.clear();
    QFile f(tagFilePath_);
    if(!f.open(QIODevice::ReadOnly))
        return;                 // no tags yet — fine
    const QByteArray raw=f.readAll();
    f.close();
    const QJsonDocument jd=QJsonDocument::fromJson(raw);
    if(!jd.isObject())
        return;
    const QJsonObject tiles=jd.object().value("tiles").toObject();
    QJsonObject::const_iterator it=tiles.constBegin();
    while(it!=tiles.constEnd())
    {
        const int id=it.key().toInt();
        const QJsonObject o=it.value().toObject();
        TileTag tag;
        tag.category=o.value("category").toString().toStdString();
        const QJsonObject attrs=o.value("attributes").toObject();
        QJsonObject::const_iterator a=attrs.constBegin();
        while(a!=attrs.constEnd())
        {
            tag.attributes[a.key().toStdString()]=a.value().toString().toStdString();
            ++a;
        }
        if(id>=0 && !tag.category.empty())
            tags_[id]=tag;
        ++it;
    }
}

// Parse the frame COUNT out of a CC "animation" property value like "100ms;6frames"
// (also "6frames", "frames=6"): the animation spans that many CONSECUTIVE tiles.
static int animationFrameCount(const QString &value)
{
    const QStringList parts=value.split(QChar(';'));
    int i=0;
    while(i<parts.size())
    {
        const QString t=parts.at(i).trimmed();
        if(t.contains("frame",Qt::CaseInsensitive))
        {
            QString digits;
            int j=0;
            while(j<t.size()) { if(t.at(j).isDigit()) digits.append(t.at(j)); j++; }
            if(!digits.isEmpty())
                return digits.toInt();
        }
        i++;
    }
    return 1;
}

// Detect SIMILAR-GROUP links (in MEMORY only — no tag merge, the tileset is never
// touched): two NON-EMPTY tiles are the SAME object on a different background when
// >=70% of their pixels match (within tolerance) over the OVERLAP — the pixels where
// BOTH have content.  So an object baked on a terrain and the same object on a
// transparent background link up.  Useful later to reason about map<->tileset
// relations.  Pairwise within the tileset.
void TagModel::detectSimilarGroups()
{
    similar_.clear();
    if(image_.isNull() || columns_<1 || tileWidth_<1 || tileHeight_<1)
        return;
    const int tw=tileWidth_,th=tileHeight_,npx=tw*th;
    const QImage cur=image_.convertToFormat(QImage::Format_ARGB32);
    // gather each NON-EMPTY tile's RGB block + an alpha>0 mask
    std::vector<int> ids;
    std::vector<std::vector<unsigned char> > rgb;
    std::vector<std::vector<char> > op;
    int id=0;
    while(id<tileCount_)
    {
        const int x0=(id%columns_)*tw;
        const int y0=(id/columns_)*th;
        if(x0+tw<=cur.width() && y0+th<=cur.height())
        {
            std::vector<unsigned char> c(npx*3);
            std::vector<char> a(npx,0);
            bool empty=true;
            int k=0,p=0,yy=0;
            while(yy<th)
            {
                const QRgb *line=reinterpret_cast<const QRgb*>(cur.scanLine(y0+yy));
                int xx=0;
                while(xx<tw)
                {
                    const QRgb px=line[x0+xx];
                    c[k++]=qRed(px); c[k++]=qGreen(px); c[k++]=qBlue(px);
                    if(qAlpha(px)!=0) { a[p]=1; empty=false; }
                    p++; xx++;
                }
                yy++;
            }
            if(!empty) { ids.push_back(id); rgb.push_back(c); op.push_back(a); }
        }
        id++;
    }
    const int TOL=18;
    const int MINOVERLAP=npx*32/256;   // a real shared region
    const int CAP=8;                   // keep the strongest few links per tile
    size_t i=0;
    while(i<ids.size())
    {
        size_t j=i+1;
        while(j<ids.size())
        {
            const std::vector<unsigned char> &A=rgb[i];
            const std::vector<unsigned char> &B=rgb[j];
            const std::vector<char> &oa=op[i];
            const std::vector<char> &ob=op[j];
            int overlap=0,match=0,p=0;
            while(p<npx)
            {
                if(oa[p] && ob[p])
                {
                    overlap++;
                    const int q=p*3;
                    if(std::abs((int)A[q]-(int)B[q])<=TOL
                       && std::abs((int)A[q+1]-(int)B[q+1])<=TOL
                       && std::abs((int)A[q+2]-(int)B[q+2])<=TOL)
                        match++;
                }
                p++;
            }
            if(overlap>=MINOVERLAP && match*100>=overlap*70)
            {
                const int pct=match*100/overlap;
                Similar sa; sa.tileId=ids[j]; sa.percent=pct; similar_[ids[i]].push_back(sa);
                Similar sb; sb.tileId=ids[i]; sb.percent=pct; similar_[ids[j]].push_back(sb);
            }
            j++;
        }
        i++;
    }
    // keep the strongest CAP links per tile (insertion sort desc; lists are short)
    std::unordered_map<int,std::vector<Similar> >::iterator it=similar_.begin();
    while(it!=similar_.end())
    {
        std::vector<Similar> &v=it->second;
        size_t a=1;
        while(a<v.size())
        {
            const Similar key=v[a];
            size_t b=a;
            while(b>0 && v[b-1].percent<key.percent) { v[b]=v[b-1]; b--; }
            v[b]=key;
            a++;
        }
        if((int)v.size()>CAP)
            v.resize(CAP);
        ++it;
    }
}

int TagModel::suggestFromTags(int minPercent)
{
    if(image_.isNull() || columns_<1 || tileWidth_<1 || tileHeight_<1)
        return 0;
    const int tw=tileWidth_,th=tileHeight_,npx=tw*th;
    const QImage cur=image_.convertToFormat(QImage::Format_ARGB32);
    // extract every NON-EMPTY tile's RGB + alpha mask, parallel arrays
    std::vector<int> ids;
    std::vector<std::vector<unsigned char> > rgb;
    std::vector<std::vector<char> > op;
    int id=0;
    while(id<tileCount_)
    {
        const int x0=(id%columns_)*tw;
        const int y0=(id/columns_)*th;
        if(x0+tw<=cur.width() && y0+th<=cur.height())
        {
            std::vector<unsigned char> c(npx*3);
            std::vector<char> a(npx,0);
            bool empty=true;
            int k=0,p=0,yy=0;
            while(yy<th)
            {
                const QRgb *line=reinterpret_cast<const QRgb*>(cur.scanLine(y0+yy));
                int xx=0;
                while(xx<tw)
                {
                    const QRgb px=line[x0+xx];
                    c[k++]=qRed(px); c[k++]=qGreen(px); c[k++]=qBlue(px);
                    if(qAlpha(px)!=0) { a[p]=1; empty=false; }
                    p++; xx++;
                }
                yy++;
            }
            if(!empty) { ids.push_back(id); rgb.push_back(c); op.push_back(a); }
        }
        id++;
    }
    const int TOL=18;
    const int MINOVERLAP=npx*32/256;
    int filled=0;
    size_t ui=0;
    while(ui<ids.size())
    {
        if(tags_.find(ids[ui])==tags_.end())   // untagged tile
        {
            int bestPct=-1;
            std::string bestCat;
            size_t ti=0;
            while(ti<ids.size())
            {
                std::unordered_map<int,TileTag>::iterator tg=tags_.find(ids[ti]);
                if(tg!=tags_.end() && !tg->second.category.empty())
                {
                    int overlap=0,match=0,p=0;
                    while(p<npx)
                    {
                        if(op[ui][p] && op[ti][p])
                        {
                            overlap++;
                            const int q=p*3;
                            if(std::abs((int)rgb[ui][q]-(int)rgb[ti][q])<=TOL
                               && std::abs((int)rgb[ui][q+1]-(int)rgb[ti][q+1])<=TOL
                               && std::abs((int)rgb[ui][q+2]-(int)rgb[ti][q+2])<=TOL)
                                match++;
                        }
                        p++;
                    }
                    if(overlap>=MINOVERLAP)
                    {
                        const int pct=match*100/overlap;
                        if(pct>bestPct) { bestPct=pct; bestCat=tg->second.category; }
                    }
                }
                ti++;
            }
            if(bestPct>=minPercent && !bestCat.empty())
            {
                TileTag tag;
                tag.category=bestCat;
                tag.attributes["auto"]="guess";
                tag.attributes["knn"]=std::to_string(bestPct);
                tags_[ids[ui]]=tag;
                filled++;
            }
        }
        ui++;
    }
    return filled;
}

const std::vector<TagModel::Similar> &TagModel::similarTo(int tileId) const
{
    static const std::vector<Similar> empty;
    std::unordered_map<int,std::vector<Similar> >::const_iterator it=similar_.find(tileId);
    return it==similar_.cend() ? empty : it->second;
}
int TagModel::similarCount() const { return (int)similar_.size(); }

bool TagModel::load(const QString &tsxPath)
{
    tsxPath_=tsxPath;
    tags_.clear();
    animatedTiles_.clear();
    similar_.clear();
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

    // read ONLY the foreign animation hint from the .tsx (read-only) — tags do NOT
    // live in the datapack; they come from the sidecar below.
    QDomElement tile=tileset.firstChildElement("tile");
    while(!tile.isNull())
    {
        const int id=tile.attribute("id","-1").toInt();
        if(id>=0)
        {
            // (a) standard Tiled animation: <tile><animation><frame tileid=.../></animation>
            QDomElement anim=tile.firstChildElement("animation");
            if(!anim.isNull())
            {
                animatedTiles_.insert(id);
                QDomElement fr=anim.firstChildElement("frame");
                while(!fr.isNull())
                {
                    const int fid=fr.attribute("tileid","-1").toInt();
                    if(fid>=0)
                        animatedTiles_.insert(fid);
                    fr=fr.nextSiblingElement("frame");
                }
            }
            // (b) CC convention: an "animation"/"frames" PROPERTY on the ANCHOR tile,
            // value "100ms;6frames" — the animation spans N CONSECUTIVE tiles, so mark
            // every frame animated (not just the anchor).
            QDomElement props=tile.firstChildElement("properties");
            if(!props.isNull())
            {
                QDomElement prop=props.firstChildElement("property");
                while(!prop.isNull())
                {
                    const QString n=prop.attribute("name");
                    if(n=="animation" || n=="frames")
                    {
                        int frames=animationFrameCount(prop.attribute("value"));
                        if(frames<1)
                            frames=1;
                        int f=0;
                        while(f<frames) { animatedTiles_.insert(id+f); f++; }
                    }
                    prop=prop.nextSiblingElement("property");
                }
            }
        }
        tile=tile.nextSiblingElement("tile");
    }
    detectSimilarGroups();   // in-memory same-object-different-bg links

    // sidecar location (OUT of the datapack), then load existing tags from it.
    // Own app subdir under the project's org dir (Qt org/app convention) so it is
    // ISOLATED from the client's data (~/.local/share/CatchChallenger/client*) —
    // never the bare org dir, and never a near-duplicate name.
    const QString root=datapackRootOf(tsxPath);
    sidecarDir_=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                +"/CatchChallenger/tileset-tagger/datapack-"+sha256Hex(QFileInfo(root).absoluteFilePath());
    tagFilePath_=sidecarDir_+"/tileset-"+sha256Hex(QFileInfo(tsxPath).absoluteFilePath())+".json";
    loadSidecarTags();

    error_.clear();
    return true;
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

void TagModel::markVerified(const std::vector<int> &tileIds)
{
    size_t i=0;
    while(i<tileIds.size())
    {
        std::unordered_map<int,TileTag>::iterator it=tags_.find(tileIds.at(i));
        if(it!=tags_.end())
            it->second.attributes.erase("auto");   // yellow -> verified
        i++;
    }
}

void TagModel::markAllVerified()
{
    std::unordered_map<int,TileTag>::iterator it=tags_.begin();
    while(it!=tags_.end())
    {
        it->second.attributes.erase("auto");
        ++it;
    }
}

std::vector<int> TagModel::unverifiedTiles() const
{
    std::vector<int> out;
    int id=0;
    while(id<tileCount_)
    {
        std::unordered_map<int,TileTag>::const_iterator it=tags_.find(id);
        if(it!=tags_.cend() && !it->second.category.empty())
        {
            if(it->second.attr("auto")=="guess")
                out.push_back(id);          // yellow: to review
        }
        else if(tileHasPixels(id))
            out.push_back(id);              // red: untagged
        id++;
    }
    return out;
}

TagModel::Counts TagModel::progress() const
{
    Counts c;
    c.verified=0;
    c.toReview=0;
    c.untagged=0;
    int id=0;
    while(id<tileCount_)
    {
        std::unordered_map<int,TileTag>::const_iterator it=tags_.find(id);
        if(it!=tags_.cend() && !it->second.category.empty())
        {
            if(it->second.attr("auto")=="guess")
                c.toReview++;
            else
                c.verified++;
        }
        else if(tileHasPixels(id))
            c.untagged++;
        id++;
    }
    return c;
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

bool TagModel::tileGreenish(int tileId) const
{
    // mean colour of the tile's opaque pixels; green-dominant + bright enough =>
    // vegetation (tree/bush/plant) rather than building/rock. A cheap heuristic
    // to split the visually-ambiguous Collisions/WalkBehind tiles.
    if(image_.isNull() || columns_<1 || tileWidth_<1 || tileHeight_<1)
        return false;
    const int x0=(tileId%columns_)*tileWidth_;
    const int y0=(tileId/columns_)*tileHeight_;
    if(x0+tileWidth_>image_.width() || y0+tileHeight_>image_.height())
        return false;
    long r=0,g=0,b=0,n=0;
    int y=0;
    while(y<tileHeight_)
    {
        int x=0;
        while(x<tileWidth_)
        {
            const QRgb p=image_.pixel(x0+x,y0+y);
            if(qAlpha(p)!=0)
            {
                r+=qRed(p);
                g+=qGreen(p);
                b+=qBlue(p);
                n++;
            }
            x++;
        }
        y++;
    }
    if(n==0)
        return false;
    r/=n;
    g/=n;
    b/=n;
    return g>r+12 && g>b+12 && g>60;
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
    // tags go to the sidecar JSON — NEVER into the datapack .tsx.
    QDir().mkpath(sidecarDir_);
    QJsonObject tiles;
    std::unordered_map<int,TileTag>::const_iterator it=tags_.begin();
    while(it!=tags_.cend())
    {
        const TileTag &tag=it->second;
        QJsonObject o;
        o.insert("category",QString::fromStdString(tag.category));
        QJsonObject attrs;
        std::map<std::string,std::string>::const_iterator a=tag.attributes.begin();
        while(a!=tag.attributes.cend())
        {
            if(!a->second.empty())
                attrs.insert(QString::fromStdString(a->first),QString::fromStdString(a->second));
            ++a;
        }
        o.insert("attributes",attrs);
        tiles.insert(QString::number(it->first),o);
        ++it;
    }
    QJsonObject root;
    root.insert("tileset",QFileInfo(tsxPath_).absoluteFilePath());
    root.insert("tiles",tiles);

    QFile out(tagFilePath_);
    if(!out.open(QIODevice::WriteOnly|QIODevice::Truncate))
    {
        error_="cannot write "+tagFilePath_;
        return false;
    }
    out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    out.close();
    error_.clear();
    return true;
}

int TagModel::tileCount() const { return tileCount_; }
int TagModel::columns() const { return columns_; }
int TagModel::tileWidth() const { return tileWidth_; }
int TagModel::tileHeight() const { return tileHeight_; }
const QImage &TagModel::image() const { return image_; }
const QString &TagModel::tagFilePath() const { return tagFilePath_; }
const QString &TagModel::tsxPath() const { return tsxPath_; }
const QString &TagModel::sidecarDir() const { return sidecarDir_; }
const QString &TagModel::error() const { return error_; }
