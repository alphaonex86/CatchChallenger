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

void TagModel::extractTiles(std::vector<int> &ids,
                            std::vector<std::vector<unsigned char> > &rgb,
                            std::vector<std::vector<char> > &op) const
{
    ids.clear(); rgb.clear(); op.clear();
    if(image_.isNull() || columns_<1 || tileWidth_<1 || tileHeight_<1)
        return;
    const int tw=tileWidth_,th=tileHeight_,npx=tw*th;
    const QImage cur=image_.convertToFormat(QImage::Format_ARGB32);
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
                    a[p]=(char)qAlpha(px);   // keep the ALPHA byte: the silhouette is a KNN signal
                    if(qAlpha(px)!=0) empty=false;
                    p++; xx++;
                }
                yy++;
            }
            if(!empty) { ids.push_back(id); rgb.push_back(c); op.push_back(a); }
        }
        id++;
    }
}

// k-NN: rank every reference tile by RGBA-match% over the UNION of opaque pixels,
// keep the top-k that clear minPercent, then VOTE — each neighbour weights its
// category by its %.  Alpha is the 4th channel: a pixel opaque in one tile but
// transparent in the other is a SILHOUETTE mismatch and lowers the score, so an
// overlay tile no longer matches a full-square wall just because their opaque
// overlap happens to share RGB.  k=3 voting beats best-1 (shrugs off a freak match).
// z-role of a tile's dominant draw layer (bottom 0 .. top 4); -1 = unknown.  The
// alpha silhouette only has meaning in the layer stack, so a tile's layer and its
// DISTANCE in z discriminate categories that happen to share RGBA.
static int layerRole(const std::string &l)
{
    if(l=="walkable" || l=="dirt") return 0;
    if(l=="grass" || l=="water" || l=="lava") return 1;
    if(l=="ledge") return 2;
    if(l=="collision") return 3;
    if(l=="over") return 4;
    return -1;
}
std::string TagModel::knnVote(size_t ui,const std::vector<size_t> &refs,
                              const std::vector<std::string> &refCat,
                              const std::vector<std::vector<unsigned char> > &rgb,
                              const std::vector<std::vector<char> > &op,
                              const std::vector<int> &refRole,int targetRole,
                              const std::vector<int> &ids,int columns,
                              int npx,int minPercent,int k,int &bestPctOut)
{
    const int TOL=18;
    const int THRA=128;                 // alpha >= THRA == opaque (RGB trustworthy)
    const int LAYERPEN=12;              // %/layer cost when target & ref sit on different z-layers
    const int SPATIALR=5, SPATIALMAX=8; // tileset-sheet proximity bonus (object-clustered layout)
    const int MINUNION=npx*32/256;
    bestPctOut=-1;
    std::vector<std::pair<int,std::string> > scored;   // (pct, category), candidates
    size_t r=0;
    while(r<refs.size())
    {
        const size_t ti=refs[r];
        if(ti!=ui)   // leave-one-out: never score a tile against itself
        {
            int uni=0,match=0,p=0;
            while(p<npx)
            {
                const int a1=(int)(unsigned char)op[ui][p];
                const int a2=(int)(unsigned char)op[ti][p];
                const bool o1=a1>=THRA, o2=a2>=THRA;
                if(o1 || o2)   // UNION: opaque in EITHER tile — the silhouette counts
                {
                    uni++;
                    bool m = std::abs(a1-a2)<=TOL;       // alpha = 4th channel (handles soft edges too)
                    if(m && o1 && o2)                    // both opaque -> RGB must also match
                    {
                        const int q=p*3;
                        m = std::abs((int)rgb[ui][q]-(int)rgb[ti][q])<=TOL
                            && std::abs((int)rgb[ui][q+1]-(int)rgb[ti][q+1])<=TOL
                            && std::abs((int)rgb[ui][q+2]-(int)rgb[ti][q+2])<=TOL;
                    }
                    if(m) match++;
                }
                p++;
            }
            if(uni>=MINUNION)
            {
                int pct=match*100/uni;
                // layer (z-order) penalty: same draw layer = no cost; farther in the
                // stack = bigger cost — the alpha silhouette is layer-relative.
                if(targetRole>=0 && refRole[r]>=0 && targetRole!=refRole[r])
                    pct -= LAYERPEN*std::abs(targetRole-refRole[r]);
                // tileset SPATIAL proximity: the sheet is laid out by object (same-
                // object tiles packed together), so a verified tile CLOSE in the sheet
                // is extra evidence for the same category.
                if(columns>0)
                {
                    const int dc=std::abs(ids[ui]%columns - ids[refs[r]]%columns);
                    const int dr=std::abs(ids[ui]/columns - ids[refs[r]]/columns);
                    const int sheetDist = dc>dr?dc:dr;     // Chebyshev distance in the sheet
                    if(sheetDist<SPATIALR)
                        pct += SPATIALMAX*(SPATIALR-sheetDist)/SPATIALR;
                }
                if(pct>=minPercent)
                    scored.push_back(std::pair<int,std::string>(pct,refCat[r]));
            }
        }
        r++;
    }
    if(scored.empty())
        return std::string();
    // partial selection-sort: bring the top-k highest % to the front
    const int kk=(k<(int)scored.size())?k:(int)scored.size();
    int sel=0;
    while(sel<kk)
    {
        int best=sel,j=sel+1;
        while(j<(int)scored.size()) { if(scored[j].first>scored[best].first) best=j; j++; }
        std::pair<int,std::string> tmp=scored[sel]; scored[sel]=scored[best]; scored[best]=tmp;
        sel++;
    }
    // weighted vote among the top-k; tie-break on the higher peak %
    std::map<std::string,int> weight,peak;
    int n=0;
    while(n<kk)
    {
        weight[scored[n].second]+=scored[n].first;
        if(scored[n].first>peak[scored[n].second]) peak[scored[n].second]=scored[n].first;
        n++;
    }
    std::string winner;
    int bestW=-1,bestPeak=-1;
    std::map<std::string,int>::iterator wi=weight.begin();
    while(wi!=weight.end())
    {
        const int pk=peak[wi->first];
        if(wi->second>bestW || (wi->second==bestW && pk>bestPeak))
        { bestW=wi->second; bestPeak=pk; winner=wi->first; }
        ++wi;
    }
    bestPctOut=peak[winner];
    return winner;
}

int TagModel::suggestFromTags(int minPercent)
{
    std::vector<int> ids;
    std::vector<std::vector<unsigned char> > rgb;
    std::vector<std::vector<char> > op;
    extractTiles(ids,rgb,op);
    if(ids.empty())
        return 0;
    const int npx=tileWidth_*tileHeight_;
    // reference set = VERIFIED tags only (no auto flag) — the human's truth
    std::vector<size_t> refs;
    std::vector<std::string> refCat;
    std::vector<int> refRole;
    size_t i=0;
    while(i<ids.size())
    {
        std::unordered_map<int,TileTag>::iterator tg=tags_.find(ids[i]);
        if(tg!=tags_.end() && !tg->second.category.empty() && tg->second.attr("auto")!="guess")
        { refs.push_back(i); refCat.push_back(tg->second.category); refRole.push_back(layerRole(tg->second.attr("layer"))); }
        i++;
    }
    if(refs.empty())
        return 0;
    const int K=3;
    int filled=0;
    size_t ui=0;
    while(ui<ids.size())
    {
        // refine tiles that are NOT human-verified: untagged OR an auto-guess
        // (heuristic/KNN) — leave the human's verified tags untouched.
        std::unordered_map<int,TileTag>::iterator cur=tags_.find(ids[ui]);
        const bool needs = (cur==tags_.end()) || (cur->second.attr("auto")=="guess");
        if(needs)
        {
            int pct=-1;
            const int targetRole = (cur!=tags_.end()) ? layerRole(cur->second.attr("layer")) : -1;
            const std::string cat=knnVote(ui,refs,refCat,rgb,op,refRole,targetRole,ids,columns_,npx,minPercent,K,pct);
            if(!cat.empty())
            {
                TileTag tag;
                tag.category=cat;
                tag.attributes["auto"]="guess";
                tag.attributes["knn"]=std::to_string(pct);
                tags_[ids[ui]]=tag;
                filled++;
            }
        }
        ui++;
    }
    return filled;
}

void TagModel::knnSelfAccuracy(int minPercent,int k,int &correct,int &total,int &abstain) const
{
    correct=0; total=0; abstain=0;
    std::vector<int> ids;
    std::vector<std::vector<unsigned char> > rgb;
    std::vector<std::vector<char> > op;
    extractTiles(ids,rgb,op);
    if(ids.empty())
        return;
    const int npx=tileWidth_*tileHeight_;
    std::vector<size_t> refs;
    std::vector<std::string> refCat;
    std::vector<int> refRole;
    size_t i=0;
    while(i<ids.size())
    {
        std::unordered_map<int,TileTag>::const_iterator tg=tags_.find(ids[i]);
        if(tg!=tags_.cend() && !tg->second.category.empty() && tg->second.attr("auto")!="guess")
        { refs.push_back(i); refCat.push_back(tg->second.category); refRole.push_back(layerRole(tg->second.attr("layer"))); }
        i++;
    }
    size_t r=0;
    while(r<refs.size())
    {
        int pct=-1;
        const std::string pred=knnVote(refs[r],refs,refCat,rgb,op,refRole,(int)refRole[r],ids,columns_,npx,minPercent,k,pct);
        total++;
        if(pred.empty())
            abstain++;
        else if(pred==refCat[r])
            correct++;
        r++;
    }
}

int TagModel::featureCount() { return 5; }
const char *TagModel::featureName(int i)
{
    static const char *n[5]={"color","shape","alpha","layer","sheet"};
    return (i>=0 && i<5) ? n[i] : "?";
}
// Emit one training row per pair of VERIFIED tiles: the LINKED-DATA feature vector
// (visual color/shape/alpha + logical layer z + tileset spatial) and the same-
// category label.  A classifier over these rows LEARNS which feature has impact,
// instead of us hand-tuning weights.
void TagModel::appendPairwiseFeatures(std::vector<std::vector<float> > &X,std::vector<char> &y) const
{
    std::vector<int> ids;
    std::vector<std::vector<unsigned char> > rgb;
    std::vector<std::vector<char> > op;
    extractTiles(ids,rgb,op);
    if(ids.empty() || columns_<1)
        return;
    const int npx=tileWidth_*tileHeight_;
    const int TOL=18, THRA=128;
    std::vector<size_t> v;
    std::vector<std::string> vcat;
    std::vector<int> vrole,vcol,vrow;
    size_t i=0;
    while(i<ids.size())
    {
        std::unordered_map<int,TileTag>::const_iterator tg=tags_.find(ids[i]);
        if(tg!=tags_.cend() && !tg->second.category.empty() && tg->second.attr("auto")!="guess")
        {
            v.push_back(i); vcat.push_back(tg->second.category);
            vrole.push_back(layerRole(tg->second.attr("layer")));
            vcol.push_back(ids[i]%columns_); vrow.push_back(ids[i]/columns_);
        }
        i++;
    }
    size_t a=0;
    while(a<v.size())
    {
        size_t b=a+1;
        while(b<v.size())
        {
            const size_t ia=v[a], ib=v[b];
            int overlap=0,uni=0,cmatch=0,aagree=0,p=0;
            while(p<npx)
            {
                const int a1=(int)(unsigned char)op[ia][p];
                const int a2=(int)(unsigned char)op[ib][p];
                const bool o1=a1>=THRA, o2=a2>=THRA;
                if(o1||o2)
                {
                    uni++;
                    if(std::abs(a1-a2)<=TOL) aagree++;
                    if(o1 && o2)
                    {
                        overlap++;
                        const int q=p*3;
                        if(std::abs((int)rgb[ia][q]-(int)rgb[ib][q])<=TOL
                           && std::abs((int)rgb[ia][q+1]-(int)rgb[ib][q+1])<=TOL
                           && std::abs((int)rgb[ia][q+2]-(int)rgb[ib][q+2])<=TOL) cmatch++;
                    }
                }
                p++;
            }
            if(uni>0)
            {
                std::vector<float> f(5);
                f[0]= overlap>0 ? (float)cmatch/(float)overlap : 0.0f;   // color  (RGB agreement on overlap)
                f[1]= (float)overlap/(float)uni;                          // shape  (silhouette IoU)
                f[2]= (float)aagree/(float)uni;                           // alpha  (silhouette agreement)
                const int dr=(vrole[a]>=0 && vrole[b]>=0) ? std::abs(vrole[a]-vrole[b]) : 2;
                f[3]= 1.0f-(float)dr/4.0f;                                // layer  (z-order closeness)
                int dc=std::abs(vcol[a]-vcol[b]), dw=std::abs(vrow[a]-vrow[b]);
                int cheb=dc>dw?dc:dw; if(cheb>10) cheb=10;
                f[4]= 1.0f-(float)cheb/10.0f;                             // sheet  (tileset proximity)
                X.push_back(f);
                y.push_back(vcat[a]==vcat[b] ? (char)1 : (char)0);
            }
            b++;
        }
        a++;
    }
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
