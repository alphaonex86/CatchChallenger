// tileset-tagger — tag a Tiled .tsx with semantic categories (table, tree-canopy,
// building-wall, ledge-down, ...) used by the learn-from-examples map generator.
// Tags live as per-tile <property> entries inside the .tsx.  GUARD: any tile that
// draws pixels but has no category is reported, so nothing is left untagged.
//
//   tileset-tagger --guard   <x.tsx>
//   tileset-tagger --tag     <x.tsx> <category> <col0> <row0> <col1> <row1> [name] [size]
//   tileset-tagger --selftest <x.tsx>
//   tileset-tagger [<x.tsx>]                  # launch the GUI (default)
//
// The GUI (rectangle-select tagging) is built on this exact TagModel core.

#include "TagModel.hpp"
#include "MainWindow.hpp"
#include "MapDecoder.hpp"
#include "MapUsageIndex.hpp"

#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QString>
#include <QStringList>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

static void printUntagged(const TagModel &model)
{
    const std::vector<int> untagged=model.untaggedNonEmpty();
    std::cout << "GUARD untagged-tiles: " << untagged.size() << " tile(s) draw pixels but have no category";
    if(!untagged.empty())
    {
        std::cout << " ->";
        size_t i=0;
        while(i<untagged.size() && i<40) { std::cout << " " << untagged.at(i); i++; }
        if(untagged.size()>40)
            std::cout << " ...";
    }
    std::cout << std::endl;
}

static int runGuard(const QString &tsx)
{
    TagModel model;
    if(!model.load(tsx)) { std::cerr << model.error().toStdString() << std::endl; return 1; }
    std::cout << tsx.toStdString() << ": " << model.tileCount() << " tiles, "
              << model.columns() << " columns, " << model.tileWidth() << "x" << model.tileHeight() << std::endl;
    printUntagged(model);
    return 0;
}

static int runTag(const QStringList &args)
{
    // args: <x.tsx> <category> <col0> <row0> <col1> <row1> [name] [size]
    if(args.size()<6) { std::cerr << "tag needs: <x.tsx> <category> <col0> <row0> <col1> <row1> [name] [size]" << std::endl; return 1; }
    TagModel model;
    if(!model.load(args.at(0))) { std::cerr << model.error().toStdString() << std::endl; return 1; }
    const std::string category=args.at(1).toStdString();
    const int c0=args.at(2).toInt(),r0=args.at(3).toInt(),c1=args.at(4).toInt(),r1=args.at(5).toInt();
    std::map<std::string,std::string> attrs;
    if(args.size()>6 && !args.at(6).isEmpty()) attrs["group"]=args.at(6).toStdString();
    if(args.size()>7 && !args.at(7).isEmpty()) attrs["size"]=args.at(7).toStdString();
    const std::vector<int> ids=model.tilesInRect(c0,r0,c1,r1);
    model.tagTiles(ids,category,attrs);
    if(!model.save()) { std::cerr << model.error().toStdString() << std::endl; return 1; }
    std::cout << "tagged " << ids.size() << " tile(s) as '" << category << "' in " << args.at(0).toStdString() << std::endl;
    printUntagged(model);
    return 0;
}

static int runSelftest(const QString &tsx)
{
    TagModel model;
    if(!model.load(tsx)) { std::cerr << "selftest load FAIL: " << model.error().toStdString() << std::endl; return 1; }
    const size_t before=model.untaggedNonEmpty().size();
    std::cout << "selftest: loaded " << model.tileCount() << " tiles, untagged-non-empty=" << before << std::endl;
    // tag 4 currently-UNTAGGED non-empty tiles so the guard count drops by exactly 4
    const std::vector<int> untagged=model.untaggedNonEmpty();
    std::vector<int> pick;
    size_t pi=0;
    while(pi<untagged.size() && pick.size()<4) { pick.push_back(untagged.at(pi)); pi++; }
    if(pick.empty()) { std::cout << "selftest: no untagged tiles with pixels (skip tag round-trip)" << std::endl; return 0; }
    std::map<std::string,std::string> attrs;
    attrs["group"]="table-0";
    attrs["size"]="2x2";
    attrs["horizontalRepeat"]="true";
    // tags go to the SIDECAR (out of the datapack) — back up any real tag file and
    // restore it so the selftest never disturbs the user's tags.
    const QString tagFile=model.tagFilePath();
    const bool had=QFile::exists(tagFile);
    QByteArray backup;
    if(had) { QFile f(tagFile); if(f.open(QIODevice::ReadOnly)) { backup=f.readAll(); f.close(); } }
    std::cout << "selftest: sidecar " << tagFile.toStdString() << std::endl;

    model.tagTiles(pick,"selftest-table",attrs);
    if(!model.save()) { std::cerr << "selftest save FAIL: " << model.error().toStdString() << std::endl; return 1; }
    // reload from the sidecar and verify tags persisted + the guard count dropped
    TagModel reload;
    if(!reload.load(tsx)) { std::cerr << "selftest reload FAIL: " << reload.error().toStdString() << std::endl; return 1; }
    bool ok=true;
    size_t k=0;
    while(k<pick.size())
    {
        if(reload.tagOf(pick.at(k)).category!="selftest-table") { ok=false; std::cerr << "  tile " << pick.at(k) << " lost its tag" << std::endl; }
        k++;
    }
    const size_t after=reload.untaggedNonEmpty().size();

    // restore the datapack sidecar to its pre-test state
    if(had) { QFile f(tagFile); if(f.open(QIODevice::WriteOnly|QIODevice::Truncate)) { f.write(backup); f.close(); } }
    else QFile::remove(tagFile);

    const bool dropOk = pick.size()<=before && after==before-pick.size();
    std::cout << "selftest: round-trip tags " << (ok?"PERSIST":"LOST") << "; untagged " << before << " -> " << after
              << " (expected -" << pick.size() << "); .tsx untouched" << std::endl;
    return (ok && dropOk) ? 0 : 1;
}

static int runUsage(const QStringList &args)
{
    // args: <x.tsx> <col0> <row0> <col1> <row1>
    if(args.size()<5) { std::cerr << "usage needs: <x.tsx> <c0> <r0> <c1> <r1>" << std::endl; return 1; }
    TagModel model;
    if(!model.load(args.at(0))) { std::cerr << model.error().toStdString() << std::endl; return 1; }
    const std::vector<int> ids=model.tilesInRect(args.at(1).toInt(),args.at(2).toInt(),args.at(3).toInt(),args.at(4).toInt());
    MapUsageIndex index;
    index.build(args.at(0));
    std::cout << index.candidateCount() << " map(s) reference " << args.at(0).toStdString() << std::endl;
    const std::vector<MapUsageIndex::Usage> usages=index.findUsages(ids);
    std::cout << "group of " << ids.size() << " tile(s) is used on " << usages.size() << " map(s):" << std::endl;
    size_t i=0;
    while(i<usages.size() && i<30)
    {
        const MapUsageIndex::Usage &u=usages.at(i);
        std::cout << "  " << u.mapLabel.toStdString() << "  " << u.cells.size() << " cell(s)  ("
                  << u.mapW << "x" << u.mapH << ")" << std::endl;
        i++;
    }
    // PRE-FILL inference data: which engine layer dominates + same-tile runs.
    const MapUsageIndex::GroupStats &st=index.lastStats();
    std::string domLayer;
    int best=-1;
    std::map<std::string,int>::const_iterator li=st.layerCounts.begin();
    while(li!=st.layerCounts.cend())
    {
        if(li->second>best) { best=li->second; domLayer=li->first; }
        ++li;
    }
    if(st.totalCells>0)
    {
        std::cout << "prefill: drawn-on layer '" << domLayer << "'  ("
                  << (best*100/st.totalCells) << "% of " << st.totalCells << " cells)"
                  << "  hRepeat=" << (st.horizontalRepeatCells*100/st.totalCells) << "%"
                  << "  vRepeat=" << (st.verticalRepeatCells*100/st.totalCells) << "%" << std::endl;
        std::cout << "effective (engine precedence): walkable=" << (st.walkableCells*100/st.totalCells)
                  << "%  blocked=" << (st.blockedCells*100/st.totalCells)
                  << "%  ledge=" << (st.ledgeCells*100/st.totalCells) << "%"
                  << "  -> " << ((st.walkableCells+st.ledgeCells>=st.blockedCells)?"WALKABLE":"BLOCKED") << std::endl;
    }
    if(!usages.empty())
    {
        // exercise the compositor on the top map (what the GUI would render)
        const QImage img=index.render(usages.at(0).mapPath);
        std::cout << "render(" << usages.at(0).mapLabel.toStdString() << ") -> "
                  << img.width() << "x" << img.height() << " px"
                  << (img.isNull() ? " [NULL]" : "") << std::endl;
    }
    return 0;
}

static int suggestOne(const QString &tsx)
{
    TagModel model;
    if(!model.load(tsx)) { std::cerr << model.error().toStdString() << std::endl; return 1; }
    MapUsageIndex index;
    index.build(tsx);
    const std::map<int,MapUsageIndex::TileStat> stats=index.analyzeAllTiles();
    int sure=0,guess=0,alreadyTagged=0;
    std::map<int,MapUsageIndex::TileStat>::const_iterator it=stats.begin();
    while(it!=stats.cend())
    {
        const int id=it->first;
        const MapUsageIndex::TileStat &s=it->second;
        if(!model.tagOf(id).category.empty()) { alreadyTagged++; ++it; continue; }
        std::string norm;
        bool walk=true,high=true;
        const std::string cat=MapUsageIndex::suggestCategory(s,model.tileGreenish(id),norm,walk,high);
        if(cat.empty()) { ++it; continue; }
        std::map<std::string,std::string> attrs;
        attrs["layer"]=norm;
        attrs["walkable"]= walk ? "true" : "false";
        if(!high)
            attrs["auto"]="guess";        // low-confidence -> flag for human review
        std::vector<int> one;
        one.push_back(id);
        model.tagTiles(one,cat,attrs);
        if(high) sure++; else guess++;
        ++it;
    }
    if(!model.save()) { std::cerr << model.error().toStdString() << std::endl; return 1; }
    const size_t remaining=model.untaggedNonEmpty().size();
    std::cout << QFileInfo(tsx).fileName().toStdString() << ": " << sure << " sure + " << guess
              << " to-review auto-tagged (" << index.candidateCount() << " maps), "
              << remaining << " untagged" << std::endl;
    return 0;
}

static int runClassify(const QStringList &args)
{
    // dry-run: show what suggest WOULD tag, without writing anything.
    if(args.isEmpty()) { std::cerr << "classify needs: <x.tsx>" << std::endl; return 1; }
    TagModel model;
    if(!model.load(args.at(0))) { std::cerr << model.error().toStdString() << std::endl; return 1; }
    MapUsageIndex index;
    index.build(args.at(0));
    const std::map<int,MapUsageIndex::TileStat> stats=index.analyzeAllTiles();
    std::map<std::string,int> hist;
    int sure=0,guess=0;
    std::map<int,MapUsageIndex::TileStat>::const_iterator it=stats.begin();
    while(it!=stats.cend())
    {
        std::string norm;
        bool walk=true,high=true;
        const std::string cat=MapUsageIndex::suggestCategory(it->second,model.tileGreenish(it->first),norm,walk,high);
        if(!cat.empty()) { hist[cat]++; if(high) sure++; else guess++; }
        ++it;
    }
    std::cout << QFileInfo(args.at(0)).fileName().toStdString() << ": " << sure << " sure + " << guess << " to-review" << std::endl;
    std::map<std::string,int>::const_iterator h=hist.begin();
    while(h!=hist.cend()) { std::cout << "  " << h->first << ": " << h->second << std::endl; ++h; }
    return 0;
}

// top-N (count,name) pairs of a histogram, descending.
static std::vector<std::pair<long,std::string> > topOf(const std::map<std::string,long> &h,size_t n)
{
    std::vector<std::pair<long,std::string> > v;
    std::map<std::string,long>::const_iterator it=h.begin();
    while(it!=h.cend()) { v.push_back(std::make_pair(it->second,it->first)); ++it; }
    std::sort(v.rbegin(),v.rend());
    if(v.size()>n)
        v.resize(n);
    return v;
}

static int runLearn(const QStringList &args)
{
    if(args.size()<2) { std::cerr << "learn needs: <map-dir> <rules.json>" << std::endl; return 1; }
    std::map<std::string,long> catCount;
    std::map<std::string,std::map<std::string,long> > rightOf,downOf;   // [A][B] = B is right/below A
    long total=0;
    int nmaps=0;
    MapDecoder dec;
    QDirIterator it(args.at(0),QStringList()<<"*.tmx",QDir::Files,QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        const QString tmx=it.next();
        MapDecoder::Result r;
        QString err;
        if(!dec.decode(tmx,r,err))
            continue;
        nmaps++;
        int y=0;
        while(y<r.h)
        {
            int x=0;
            while(x<r.w)
            {
                const std::string &a=r.categoryGrid.at(x+y*r.w);
                if(!a.empty())
                {
                    catCount[a]++;
                    total++;
                    if(x+1<r.w) { const std::string &b=r.categoryGrid.at((x+1)+y*r.w); if(!b.empty()) rightOf[a][b]++; }
                    if(y+1<r.h) { const std::string &b=r.categoryGrid.at(x+(y+1)*r.w); if(!b.empty()) downOf[a][b]++; }
                }
                x++;
            }
            y++;
        }
    }
    if(total==0) { std::cerr << "learn: no decodable maps under " << args.at(0).toStdString() << std::endl; return 1; }

    // write rules.json
    {
        QJsonObject cats;
        std::map<std::string,long>::const_iterator c=catCount.begin();
        while(c!=catCount.cend()) { cats.insert(QString::fromStdString(c->first),(double)c->second); ++c; }
        QJsonObject jright,jdown;
        std::map<std::string,std::map<std::string,long> >::const_iterator a=rightOf.begin();
        while(a!=rightOf.cend())
        {
            QJsonObject row;
            std::map<std::string,long>::const_iterator b=a->second.begin();
            while(b!=a->second.cend()) { row.insert(QString::fromStdString(b->first),(double)b->second); ++b; }
            jright.insert(QString::fromStdString(a->first),row);
            ++a;
        }
        a=downOf.begin();
        while(a!=downOf.cend())
        {
            QJsonObject row;
            std::map<std::string,long>::const_iterator b=a->second.begin();
            while(b!=a->second.cend()) { row.insert(QString::fromStdString(b->first),(double)b->second); ++b; }
            jdown.insert(QString::fromStdString(a->first),row);
            ++a;
        }
        QJsonObject root;
        root.insert("maps",nmaps);
        root.insert("cells",(double)total);
        root.insert("categories",cats);
        root.insert("rightOf",jright);
        root.insert("downOf",jdown);
        QFile out(args.at(1));
        if(!out.open(QIODevice::WriteOnly|QIODevice::Truncate)) { std::cerr << "cannot write " << args.at(1).toStdString() << std::endl; return 1; }
        out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        out.close();
    }

    // human-readable summary (sanity check the learned rules)
    std::cout << "learned from " << nmaps << " maps, " << total << " cells, "
              << catCount.size() << " categories -> " << args.at(1).toStdString() << std::endl;
    const std::vector<std::pair<long,std::string> > top=topOf(catCount,12);
    std::cout << "top categories:";
    size_t i=0;
    while(i<top.size()) { std::cout << " " << top.at(i).second << "(" << (top.at(i).first*100/total) << "%)"; i++; }
    std::cout << std::endl;
    std::cout << "dominant neighbours (right → / down ↓):" << std::endl;
    i=0;
    while(i<top.size())
    {
        const std::string &cat=top.at(i).second;
        const std::vector<std::pair<long,std::string> > r=topOf(rightOf[cat],3);
        const std::vector<std::pair<long,std::string> > d=topOf(downOf[cat],3);
        std::cout << "  " << cat << "  → ";
        size_t k=0; while(k<r.size()) { std::cout << r.at(k).second << " "; k++; }
        std::cout << "  ↓ ";
        k=0; while(k<d.size()) { std::cout << d.at(k).second << " "; k++; }
        std::cout << std::endl;
        i++;
    }
    // hard constraints: pairs of common categories that are NEVER adjacent
    std::cout << "never-adjacent (hard constraints) among the top categories:" << std::endl;
    int shown=0;
    size_t ai=0;
    while(ai<top.size() && shown<14)
    {
        size_t bi=ai+1;
        while(bi<top.size() && shown<14)
        {
            const std::string &A=top.at(ai).second;
            const std::string &B=top.at(bi).second;
            const long n = rightOf[A][B]+rightOf[B][A]+downOf[A][B]+downOf[B][A];
            if(n==0) { std::cout << "  " << A << " | " << B << std::endl; shown++; }
            bi++;
        }
        ai++;
    }
    return 0;
}

// ---- generator (A): rigid rectangles from the learned structure --------------
static void parseWxH(const std::string &k,int &w,int &h)
{
    const size_t p=k.find('x');
    if(p==std::string::npos) { w=2; h=2; return; }
    w=std::atoi(k.substr(0,p).c_str());
    h=std::atoi(k.substr(p+1).c_str());
    if(w<1) w=1;
    if(h<1) h=1;
}

static std::string sampleSize(const std::map<std::string,long> &hist,std::mt19937 &rng)
{
    long total=0;
    std::map<std::string,long>::const_iterator it=hist.begin();
    while(it!=hist.cend()) { total+=it->second; ++it; }
    if(total<=0)
        return "2x2";
    std::uniform_int_distribution<long> d(0,total-1);
    long r=d(rng);
    it=hist.begin();
    while(it!=hist.cend()) { if(r<it->second) return it->first; r-=it->second; ++it; }
    return hist.begin()->first;
}

static bool loadStruct(const QString &path,std::map<std::string,long> &buildingSizes,
                       std::map<std::string,std::map<std::string,long> > &zoneSizes)
{
    QFile f(path);
    if(!f.open(QIODevice::ReadOnly))
        return false;
    const QJsonDocument jd=QJsonDocument::fromJson(f.readAll());
    f.close();
    if(!jd.isObject())
        return false;
    const QJsonObject root=jd.object();
    const QJsonObject bsz=root.value("buildings").toObject().value("sizes").toObject();
    QJsonObject::const_iterator b=bsz.constBegin();
    while(b!=bsz.constEnd()) { buildingSizes[b.key().toStdString()]=(long)b.value().toDouble(); ++b; }
    const QJsonObject zones=root.value("zones").toObject();
    QJsonObject::const_iterator z=zones.constBegin();
    while(z!=zones.constEnd())
    {
        const QJsonObject szs=z.value().toObject().value("sizes").toObject();
        QJsonObject::const_iterator s=szs.constBegin();
        while(s!=szs.constEnd()) { zoneSizes[z.key().toStdString()][s.key().toStdString()]=(long)s.value().toDouble(); ++s; }
        ++z;
    }
    return true;
}

static bool rectFree(const std::vector<char> &occ,int W,int H,int x,int y,int w,int h)
{
    if(x<0 || y<0 || x+w>W || y+h>H)
        return false;
    int j=0;
    while(j<h) { int i=0; while(i<w) { if(occ[(x+i)+(y+j)*W]!=0) return false; i++; } j++; }
    return true;
}

static void fillRect(std::vector<std::string> &grid,std::vector<char> &occ,int W,int x,int y,int w,int h,const std::string &cat)
{
    int j=0;
    while(j<h) { int i=0; while(i<w) { const int c=(x+i)+(y+j)*W; grid[c]=cat; occ[c]=1; i++; } j++; }
}

static int runGenerate(const QStringList &args)
{
    if(args.size()<2) { std::cerr << "generate needs: <struct.json> <out.png> [W H seed]" << std::endl; return 1; }
    std::map<std::string,long> buildingSizes;
    std::map<std::string,std::map<std::string,long> > zoneSizes;
    if(!loadStruct(args.at(0),buildingSizes,zoneSizes)) { std::cerr << "cannot load " << args.at(0).toStdString() << std::endl; return 1; }
    const int W = args.size()>2 ? args.at(2).toInt() : 40;
    const int H = args.size()>3 ? args.at(3).toInt() : 30;
    const unsigned seed = args.size()>4 ? (unsigned)args.at(4).toUInt() : 1u;
    std::mt19937 rng(seed);

    const std::string base = zoneSizes.count("grass-tall")>0 ? "grass-tall" : "ground";
    std::vector<std::string> grid(W*H,base);
    std::vector<char> occ(W*H,0);

    // tree border (the enclosed overworld look)
    {
        int y=0;
        while(y<H) { int x=0; while(x<W) { if(x<2||y<2||x>=W-2||y>=H-2) { grid[x+y*W]="tree-trunk"; occ[x+y*W]=1; } x++; } y++; }
    }

    // helper-free placement of N rectangles of a zone category, sampled sizes
    const char *zonePlace[3]={"water","grass-short","tree-trunk"};
    const int zoneN[3]={ (W*H)/600+1, (W*H)/300+1, (W*H)/250+1 };
    int zk=0;
    while(zk<3)
    {
        if(zoneSizes.count(zonePlace[zk])>0)
        {
            int placed=0;
            int tries=0;
            while(placed<zoneN[zk] && tries<zoneN[zk]*20)
            {
                int w=2,h=2;
                parseWxH(sampleSize(zoneSizes[zonePlace[zk]],rng),w,h);
                if(w<=W-4 && h<=H-4)
                {
                    std::uniform_int_distribution<int> dx(2,W-2-w),dy(2,H-2-h);
                    const int x=dx(rng),y=dy(rng);
                    if(rectFree(occ,W,H,x,y,w,h)) { fillRect(grid,occ,W,x,y,w,h,zonePlace[zk]); placed++; }
                }
                tries++;
            }
        }
        zk++;
    }

    // buildings: wall block, roof on the top row, door at the bottom-centre
    {
        const int nBuild=(W*H)/200+2;
        int placed=0,tries=0;
        while(placed<nBuild && tries<nBuild*30)
        {
            int w=4,h=2;
            parseWxH(sampleSize(buildingSizes,rng),w,h);
            if(w>=2 && h>=2 && w<=W-6 && h<=H-6)
            {
                std::uniform_int_distribution<int> dx(3,W-3-w),dy(3,H-3-h);
                const int x=dx(rng),y=dy(rng);
                if(rectFree(occ,W,H,x,y,w,h))
                {
                    fillRect(grid,occ,W,x,y,w,h,"building-wall");
                    int i=0; while(i<w) { grid[(x+i)+y*W]="building-roof"; i++; }   // top row roof
                    grid[(x+w/2)+(y+h-1)*W]="door";                                  // door bottom-centre
                    placed++;
                }
            }
            tries++;
        }
    }

    // render
    const int cell=12;
    QImage img(W*cell,H*cell,QImage::Format_ARGB32);
    {
        QPainter p(&img);
        int y=0;
        while(y<H) { int x=0; while(x<W) { p.fillRect(x*cell,y*cell,cell,cell,MapDecoder::categoryColor(grid[x+y*W])); x++; } y++; }
    }
    if(!img.save(args.at(1))) { std::cerr << "cannot write " << args.at(1).toStdString() << std::endl; return 1; }
    std::cout << "generated " << W << "x" << H << " (seed " << seed << ") -> " << args.at(1).toStdString() << std::endl;
    return 0;
}

// ---- structure learning (B): rigid rectangular zones + building templates ---
struct Comp { int minx; int miny; int maxx; int maxy; int count; };

static std::vector<Comp> components(const std::vector<std::string> &grid,int w,int h,const std::set<std::string> &cats)
{
    std::vector<char> seen(w*h,0);
    std::vector<Comp> out;
    int idx=0;
    while(idx<w*h)
    {
        if(seen[idx]==0 && grid.at(idx).size()>0 && cats.count(grid.at(idx))>0)
        {
            std::vector<int> st;
            st.push_back(idx);
            seen[idx]=1;
            Comp c;
            c.minx=c.maxx=idx%w;
            c.miny=c.maxy=idx/w;
            c.count=0;
            while(!st.empty())
            {
                const int p=st.back();
                st.pop_back();
                c.count++;
                const int px=p%w,py=p/w;
                if(px<c.minx) c.minx=px;
                if(px>c.maxx) c.maxx=px;
                if(py<c.miny) c.miny=py;
                if(py>c.maxy) c.maxy=py;
                const int nb[4]={px>0?p-1:-1, px<w-1?p+1:-1, py>0?p-w:-1, py<h-1?p+w:-1};
                int k=0;
                while(k<4)
                {
                    const int q=nb[k];
                    if(q>=0 && seen[q]==0 && grid.at(q).size()>0 && cats.count(grid.at(q))>0) { seen[q]=1; st.push_back(q); }
                    k++;
                }
            }
            out.push_back(c);
        }
        idx++;
    }
    return out;
}

static std::string sizeKey(const Comp &c)
{
    return std::to_string(c.maxx-c.minx+1)+"x"+std::to_string(c.maxy-c.miny+1);
}

static int runStructure(const QStringList &args)
{
    if(args.size()<2) { std::cerr << "structure needs: <map-dir> <struct.json>" << std::endl; return 1; }
    std::set<std::string> buildingCats;
    buildingCats.insert("building-wall");
    buildingCats.insert("building-roof");
    buildingCats.insert("door");
    buildingCats.insert("window");
    const char *zoneCatsArr[8]={"water","grass-tall","grass-short","sand","ground","path","cliff","tree-trunk"};

    long nBuildings=0;
    long fillSum=0;        // for avg rectangle-fill of buildings
    std::map<std::string,long> buildingSizes;
    std::map<std::string,std::map<std::string,long> > zoneSizes;
    std::map<std::string,long> zoneCounts;
    int nmaps=0;
    MapDecoder dec;
    QDirIterator it(args.at(0),QStringList()<<"*.tmx",QDir::Files,QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        const QString tmx=it.next();
        MapDecoder::Result r;
        QString err;
        if(!dec.decode(tmx,r,err))
            continue;
        nmaps++;
        const std::vector<Comp> bc=components(r.categoryGrid,r.w,r.h,buildingCats);
        size_t i=0;
        while(i<bc.size())
        {
            const Comp &c=bc.at(i);
            if(c.count>=4)
            {
                nBuildings++;
                buildingSizes[sizeKey(c)]++;
                const int area=(c.maxx-c.minx+1)*(c.maxy-c.miny+1);
                if(area>0) fillSum += c.count*100/area;
            }
            i++;
        }
        int z=0;
        while(z<8)
        {
            std::set<std::string> one;
            one.insert(zoneCatsArr[z]);
            const std::vector<Comp> zc=components(r.categoryGrid,r.w,r.h,one);
            size_t j=0;
            while(j<zc.size())
            {
                if(zc.at(j).count>=4) { zoneCounts[zoneCatsArr[z]]++; zoneSizes[zoneCatsArr[z]][sizeKey(zc.at(j))]++; }
                j++;
            }
            z++;
        }
    }
    if(nmaps==0) { std::cerr << "structure: no decodable maps" << std::endl; return 1; }

    // write struct.json
    {
        QJsonObject bsz;
        std::map<std::string,long>::const_iterator b=buildingSizes.begin();
        while(b!=buildingSizes.cend()) { bsz.insert(QString::fromStdString(b->first),(double)b->second); ++b; }
        QJsonObject buildings;
        buildings.insert("count",(double)nBuildings);
        buildings.insert("avgRectFillPercent", nBuildings>0 ? (double)(fillSum/nBuildings) : 0.0);
        buildings.insert("sizes",bsz);
        QJsonObject zones;
        std::map<std::string,std::map<std::string,long> >::const_iterator zc=zoneSizes.begin();
        while(zc!=zoneSizes.cend())
        {
            QJsonObject szrow;
            std::map<std::string,long>::const_iterator s=zc->second.begin();
            while(s!=zc->second.cend()) { szrow.insert(QString::fromStdString(s->first),(double)s->second); ++s; }
            QJsonObject zob;
            zob.insert("count",(double)zoneCounts[zc->first]);
            zob.insert("sizes",szrow);
            zones.insert(QString::fromStdString(zc->first),zob);
            ++zc;
        }
        QJsonObject root;
        root.insert("maps",nmaps);
        root.insert("buildings",buildings);
        root.insert("zones",zones);
        QFile out(args.at(1));
        if(!out.open(QIODevice::WriteOnly|QIODevice::Truncate)) { std::cerr << "cannot write " << args.at(1).toStdString() << std::endl; return 1; }
        out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        out.close();
    }

    // summary
    std::cout << "structure from " << nmaps << " maps -> " << args.at(1).toStdString() << std::endl;
    std::cout << "buildings: " << nBuildings << " instances, avg rect-fill "
              << (nBuildings>0 ? fillSum/nBuildings : 0) << "%  top sizes:";
    const std::vector<std::pair<long,std::string> > bt=topOf(buildingSizes,6);
    size_t i=0;
    while(i<bt.size()) { std::cout << " " << bt.at(i).second << "(" << bt.at(i).first << ")"; i++; }
    std::cout << std::endl;
    std::cout << "feature zones (count, top sizes):" << std::endl;
    int z=0;
    while(z<8)
    {
        const std::string cat=zoneCatsArr[z];
        if(zoneCounts.count(cat)>0)
        {
            std::cout << "  " << cat << ": " << zoneCounts[cat] << "  ";
            const std::vector<std::pair<long,std::string> > zt=topOf(zoneSizes[cat],5);
            size_t k=0;
            while(k<zt.size()) { std::cout << zt.at(k).second << "(" << zt.at(k).first << ") "; k++; }
            std::cout << std::endl;
        }
        z++;
    }
    return 0;
}

// ---- reproducibility guard -------------------------------------------------
// The generator visits cells in raster order; at each cell the ALLOWED set is the
// categories compatible with the already-placed left + up neighbours per the
// learned adjacency rules (count >= threshold). encode() walks a real map the same
// way and records, per cell, the index of the real category in its allowed set
// (the "choice"); replay() re-runs the generator with those choices and must
// reproduce the map IDENTICALLY. coverage = % cells the rules could choose.

static bool loadRules(const QString &path,std::vector<std::string> &catList,
                      std::map<std::string,std::map<std::string,long> > &rightOf,
                      std::map<std::string,std::map<std::string,long> > &downOf)
{
    QFile f(path);
    if(!f.open(QIODevice::ReadOnly))
        return false;
    const QJsonDocument jd=QJsonDocument::fromJson(f.readAll());
    f.close();
    if(!jd.isObject())
        return false;
    const QJsonObject root=jd.object();
    const QJsonObject cats=root.value("categories").toObject();
    QJsonObject::const_iterator c=cats.constBegin();
    while(c!=cats.constEnd()) { catList.push_back(c.key().toStdString()); ++c; }
    std::sort(catList.begin(),catList.end());
    const char *dirs[2]={"rightOf","downOf"};
    int d=0;
    while(d<2)
    {
        const QJsonObject mat=root.value(dirs[d]).toObject();
        std::map<std::string,std::map<std::string,long> > &dst = d==0 ? rightOf : downOf;
        QJsonObject::const_iterator a=mat.constBegin();
        while(a!=mat.constEnd())
        {
            const QJsonObject row=a.value().toObject();
            QJsonObject::const_iterator b=row.constBegin();
            while(b!=row.constEnd()) { dst[a.key().toStdString()][b.key().toStdString()]=(long)b.value().toDouble(); ++b; }
            ++a;
        }
        d++;
    }
    return true;
}

static long adjCount(const std::map<std::string,std::map<std::string,long> > &m,
                     const std::string &a,const std::string &b)
{
    std::map<std::string,std::map<std::string,long> >::const_iterator it=m.find(a);
    if(it==m.cend())
        return 0;
    std::map<std::string,long>::const_iterator jt=it->second.find(b);
    return jt==it->second.cend() ? 0 : jt->second;
}

static std::vector<std::string> allowedAt(const std::vector<std::string> &grid,int x,int y,int w,
                                          const std::vector<std::string> &catList,
                                          const std::map<std::string,std::map<std::string,long> > &rightOf,
                                          const std::map<std::string,std::map<std::string,long> > &downOf,long threshold)
{
    const std::string left = x>0 ? grid.at((x-1)+y*w) : std::string();
    const std::string up   = y>0 ? grid.at(x+(y-1)*w) : std::string();
    std::vector<std::string> out;
    size_t i=0;
    while(i<catList.size())
    {
        const std::string &C=catList.at(i);
        bool ok=true;
        if(!left.empty() && adjCount(rightOf,left,C)<threshold) ok=false;       // empty neighbour = border, no constraint
        if(ok && !up.empty() && adjCount(downOf,up,C)<threshold) ok=false;
        if(ok) out.push_back(C);
        i++;
    }
    return out;
}

static int runVerify(const QStringList &args)
{
    if(args.size()<2) { std::cerr << "verify needs: <map-dir> <rules.json> [threshold]" << std::endl; return 1; }
    const long threshold = args.size()>2 ? args.at(2).toLong() : 1;
    std::vector<std::string> catList;
    std::map<std::string,std::map<std::string,long> > rightOf,downOf;
    if(!loadRules(args.at(1),catList,rightOf,downOf)) { std::cerr << "cannot load rules " << args.at(1).toStdString() << std::endl; return 1; }

    MapDecoder dec;
    QDirIterator it(args.at(0),QStringList()<<"*.tmx",QDir::Files,QDirIterator::Subdirectories);
    int nmaps=0,identicalMaps=0;
    long totalCells=0,coveredCells=0;
    std::map<std::string,long> gapPairs;     // "L>C" / "U/C" adjacency the threshold pruned
    while(it.hasNext())
    {
        const QString tmx=it.next();
        MapDecoder::Result r;
        QString err;
        if(!dec.decode(tmx,r,err))
            continue;
        nmaps++;
        const int w=r.w,h=r.h;
        // encode (place real cats; record choice index in the allowed set)
        std::vector<int> choice(w*h,-2);          // -2 empty, -1 escape, >=0 allowed index
        std::vector<std::string> escapeCat;
        long mapCovered=0,mapCells=0;
        {
            int y=0;
            while(y<h)
            {
                int x=0;
                while(x<w)
                {
                    const std::string &C=r.categoryGrid.at(x+y*w);
                    if(!C.empty())
                    {
                        mapCells++;
                        const std::vector<std::string> allow=allowedAt(r.categoryGrid,x,y,w,catList,rightOf,downOf,threshold);
                        int k=-1,j=0;
                        while(j<(int)allow.size()) { if(allow.at(j)==C) { k=j; break; } j++; }
                        if(k>=0) { choice[x+y*w]=k; mapCovered++; }
                        else
                        {
                            choice[x+y*w]=-1;
                            escapeCat.push_back(C);
                            // record which adjacency the pruned rules could not admit
                            if(x>0) gapPairs[r.categoryGrid.at((x-1)+y*w)+" → "+C]++;
                            else if(y>0) gapPairs[r.categoryGrid.at(x+(y-1)*w)+" ↓ "+C]++;
                        }
                    }
                    x++;
                }
                y++;
            }
        }
        // replay (reconstruct from the choices; must equal the real grid)
        std::vector<std::string> recon(w*h);
        size_t escI=0;
        bool identical=true;
        {
            int y=0;
            while(y<h)
            {
                int x=0;
                while(x<w)
                {
                    const int ch=choice[x+y*w];
                    std::string out;
                    if(ch==-2)
                        out=std::string();
                    else if(ch==-1)
                        out = escI<escapeCat.size() ? escapeCat.at(escI++) : std::string();
                    else
                    {
                        const std::vector<std::string> allow=allowedAt(recon,x,y,w,catList,rightOf,downOf,threshold);
                        out = ch<(int)allow.size() ? allow.at(ch) : std::string("?");
                    }
                    recon[x+y*w]=out;
                    if(out!=r.categoryGrid.at(x+y*w))
                        identical=false;
                    x++;
                }
                y++;
            }
        }
        if(identical)
            identicalMaps++;
        else
            std::cerr << "  NOT IDENTICAL (framework bug): " << QFileInfo(tmx).fileName().toStdString() << std::endl;
        totalCells+=mapCells;
        coveredCells+=mapCovered;
    }
    if(nmaps==0) { std::cerr << "verify: no decodable maps" << std::endl; return 1; }

    const int idPct = identicalMaps*100/nmaps;
    const long covPct = totalCells>0 ? coveredCells*100/totalCells : 0;
    std::cout << "REPRODUCIBILITY GUARD (threshold=" << threshold << "):" << std::endl;
    std::cout << "  identical replay: " << identicalMaps << "/" << nmaps << " maps (" << idPct << "%)"
              << (idPct==100 ? "  [SOUND]" : "  [FRAMEWORK BUG]") << std::endl;
    std::cout << "  rule coverage: " << covPct << "% of " << totalCells << " cells chosen by the rules ("
              << (totalCells-coveredCells) << " escapes)" << std::endl;
    if(!gapPairs.empty())
    {
        const std::vector<std::pair<long,std::string> > top=topOf(gapPairs,8);
        std::cout << "  top adjacencies the threshold could not admit:" << std::endl;
        size_t i=0;
        while(i<top.size()) { std::cout << "    " << top.at(i).second << " (" << top.at(i).first << ")" << std::endl; i++; }
    }
    return 0;
}

static int decodeOne(const QString &mapPath,const QString &outPng)
{
    MapDecoder dec;
    MapDecoder::Result r;
    QString err;
    if(!dec.decode(mapPath,r,err)) { std::cerr << "decode FAIL " << mapPath.toStdString() << ": " << err.toStdString() << std::endl; return 1; }
    // side-by-side: real | category
    const int gap=8;
    QImage combined(r.realRender.width()+gap+r.categoryRender.width(),
                    r.realRender.height(),QImage::Format_ARGB32);
    combined.fill(QColor(0,0,0));
    {
        QPainter p(&combined);
        p.drawImage(0,0,r.realRender);
        p.drawImage(r.realRender.width()+gap,0,r.categoryRender);
    }
    if(!combined.save(outPng)) { std::cerr << "cannot write " << outPng.toStdString() << std::endl; return 1; }
    const int cov = r.totalDrawn>0 ? r.tagged*100/r.totalDrawn : 0;
    std::cout << QFileInfo(mapPath).fileName().toStdString() << ": " << r.w << "x" << r.h
              << "  coverage " << cov << "% (" << r.tagged << " tagged / " << r.untagged << " untagged of "
              << r.totalDrawn << " drawn)  -> " << outPng.toStdString() << std::endl;
    // top categories by cell count, to cross-check the render
    std::map<std::string,int> hist;
    size_t gi=0;
    while(gi<r.categoryGrid.size()) { if(!r.categoryGrid.at(gi).empty()) hist[r.categoryGrid.at(gi)]++; gi++; }
    std::vector<std::pair<int,std::string> > top;
    std::map<std::string,int>::const_iterator hit=hist.begin();
    while(hit!=hist.cend()) { top.push_back(std::make_pair(hit->second,hit->first)); ++hit; }
    std::sort(top.rbegin(),top.rend());
    std::cout << "   ";
    size_t k=0;
    while(k<top.size() && k<8) { std::cout << top.at(k).second << "=" << top.at(k).first << "  "; k++; }
    std::cout << std::endl;
    return 0;
}

static int runDecode(const QStringList &args)
{
    if(args.size()<2) { std::cerr << "decode needs: <map.tmx | map-dir> <out.png | out-dir>" << std::endl; return 1; }
    const QFileInfo fi(args.at(0));
    if(fi.isDir())
    {
        QDir().mkpath(args.at(1));
        QDirIterator it(args.at(0),QStringList()<<"*.tmx",QDir::Files,QDirIterator::Subdirectories);
        int rc=0,n=0;
        while(it.hasNext())
        {
            const QString tmx=it.next();
            const QString png=args.at(1)+"/"+QFileInfo(tmx).completeBaseName()+".png";
            if(decodeOne(tmx,png)!=0) rc=1;
            n++;
        }
        std::cout << "decoded " << n << " map(s) -> " << args.at(1).toStdString() << std::endl;
        return rc;
    }
    return decodeOne(args.at(0),args.at(1));
}

static int runSuggest(const QStringList &args)
{
    if(args.isEmpty()) { std::cerr << "suggest needs: <x.tsx | tileset-dir>" << std::endl; return 1; }
    const QFileInfo fi(args.at(0));
    if(fi.isDir())
    {
        QDir d(args.at(0));
        const QStringList tsxs=d.entryList(QStringList()<<"*.tsx",QDir::Files,QDir::Name);
        std::cout << "suggest: " << tsxs.size() << " tileset(s) in " << args.at(0).toStdString() << std::endl;
        int rc=0,k=0;
        while(k<tsxs.size()) { if(suggestOne(d.absoluteFilePath(tsxs.at(k)))!=0) rc=1; k++; }
        std::cout << "done — open each in the GUI to set the visually-ambiguous categories." << std::endl;
        return rc;
    }
    return suggestOne(args.at(0));
}

int main(int argc, char *argv[])
{
    QStringList args;
    int i=1;
    while(i<argc) { args.append(QString::fromUtf8(argv[i])); i++; }
    const bool cli = !args.isEmpty()
        && (args.at(0)=="--guard" || args.at(0)=="--tag" || args.at(0)=="--selftest"
            || args.at(0)=="--usage" || args.at(0)=="--suggest" || args.at(0)=="--classify"
            || args.at(0)=="--decode" || args.at(0)=="--learn" || args.at(0)=="--verify" || args.at(0)=="--structure" || args.at(0)=="--generate");
    if(cli)
        qputenv("QT_QPA_PLATFORM","offscreen"); //headless: no display needed

    QApplication app(argc,argv);

    if(!args.isEmpty() && args.at(0)=="--guard" && args.size()>=2)
        return runGuard(args.at(1));
    if(!args.isEmpty() && args.at(0)=="--tag")
        return runTag(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--selftest" && args.size()>=2)
        return runSelftest(args.at(1));
    if(!args.isEmpty() && args.at(0)=="--usage")
        return runUsage(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--suggest")
        return runSuggest(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--classify")
        return runClassify(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--decode")
        return runDecode(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--learn")
        return runLearn(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--verify")
        return runVerify(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--structure")
        return runStructure(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--generate")
        return runGenerate(args.mid(1));

    // GUI: optional first arg is a .tsx to open.
    MainWindow window;
    if(!args.isEmpty() && !args.at(0).startsWith("--"))
        window.openTsx(args.at(0));
    window.show();
    return app.exec();
}
