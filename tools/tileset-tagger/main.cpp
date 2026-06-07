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

#include "grouplayer.h"
#include "layer.h"
#include "map.h"
#include "mapreader.h"
#include "tilelayer.h"
#include "tileset.h"
#include <cstdio>
#include <memory>

#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QString>
#include <QStringList>
#include <QTextStream>
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
    const TagModel::Counts c=model.progress();
    const int total=c.verified+c.toReview+c.untagged;
    std::cout << "progress: " << (total>0?c.verified*100/total:100) << "%  verified=" << c.verified
              << "  to-review=" << c.toReview << "  untagged=" << c.untagged << std::endl;
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

// dry-run per-tile heuristic prediction (id<TAB>category<TAB>sure|guess), no write —
// for evaluating the auto-tagger against hand tags.
static int runEvalSuggest(const QStringList &args)
{
    if(args.isEmpty()) { std::cerr << "evalsuggest needs: <x.tsx>" << std::endl; return 1; }
    TagModel model;
    if(!model.load(args.at(0))) { std::cerr << model.error().toStdString() << std::endl; return 1; }
    MapUsageIndex index;
    index.build(args.at(0));
    const std::map<int,MapUsageIndex::TileStat> stats=index.analyzeAllTiles();
    std::map<int,MapUsageIndex::TileStat>::const_iterator it=stats.begin();
    while(it!=stats.cend())
    {
        std::string norm;
        bool walk=true,high=true;
        const std::string cat=MapUsageIndex::suggestCategory(it->second,model.tileGreenish(it->first),norm,walk,high);
        std::cout << it->first << "\t" << (cat.empty()?std::string("?"):cat) << "\t" << (high?"sure":"guess") << std::endl;
        ++it;
    }
    return 0;
}

// Measure the KNN auto-detect by LEAVE-ONE-OUT over the VERIFIED tags: predict
// each verified tile from the others, count hits.  Sweeps k x minPercent so the
// defaults (k=3, minPercent>=55) can be tuned on real data.  No write.
static int runEvalKnn(const QStringList &args)
{
    if(args.isEmpty()) { std::cerr << "evalknn needs: <x.tsx | tileset-dir>" << std::endl; return 1; }
    QStringList tsx;
    QFileInfo fi(args.at(0));
    if(fi.isDir())
    {
        QDirIterator it(args.at(0),QStringList()<<"*.tsx",QDir::Files,QDirIterator::Subdirectories);
        while(it.hasNext()) tsx.append(it.next());
        tsx.sort();
    }
    else
        tsx.append(args.at(0));
    const int ks[3]={1,3,5};
    const int mps[4]={45,55,65,75};
    // accumulate correct/total/abstain per (k,mp) across all tilesets
    int accCorrect[3][4],accTotal[3][4],accAbstain[3][4];
    int a=0; while(a<3){int b=0;while(b<4){accCorrect[a][b]=0;accTotal[a][b]=0;accAbstain[a][b]=0;b++;}a++;}
    int loaded=0;
    int ti=0;
    while(ti<tsx.size())
    {
        TagModel model;
        if(model.load(tsx.at(ti)))
        {
            const TagModel::Counts c=model.progress();
            if(c.verified>0)
            {
                loaded++;
                int ki=0;
                while(ki<3)
                {
                    int mi=0;
                    while(mi<4)
                    {
                        int correct=0,total=0,abstain=0;
                        model.knnSelfAccuracy(mps[mi],ks[ki],correct,total,abstain);
                        accCorrect[ki][mi]+=correct; accTotal[ki][mi]+=total; accAbstain[ki][mi]+=abstain;
                        mi++;
                    }
                    ki++;
                }
                if(tsx.size()>1)
                {
                    int correct=0,total=0,abstain=0;
                    model.knnSelfAccuracy(55,3,correct,total,abstain);
                    const int pct=total>0?correct*100/total:0;
                    std::cout << QFileInfo(tsx.at(ti)).fileName().toStdString() << "\tk3/55: "
                              << correct << "/" << total << " = " << pct << "%  (abstain " << abstain << ")" << std::endl;
                }
            }
        }
        ti++;
    }
    if(loaded==0) { std::cerr << "no tileset with verified tags found" << std::endl; return 1; }
    std::cout << "\n== leave-one-out KNN accuracy over " << loaded << " tileset(s), "
              << "correct / (total - abstain) ==" << std::endl;
    std::cout << "         ";
    int mi=0; while(mi<4){ std::cout << "  >=" << mps[mi] << "%"; mi++; } std::cout << std::endl;
    int ki=0;
    while(ki<3)
    {
        std::cout << "  k=" << ks[ki] << "   ";
        mi=0;
        while(mi<4)
        {
            const int answered=accTotal[ki][mi]-accAbstain[ki][mi];
            const int pct=answered>0?accCorrect[ki][mi]*100/answered:0;
            char buf[16]; std::snprintf(buf,sizeof(buf),"   %3d%%",pct);
            std::cout << buf;
            mi++;
        }
        std::cout << std::endl;
        ki++;
    }
    // also report coverage (answered fraction) for the recommended cell
    const int ansR=accTotal[1][1]-accAbstain[1][1];
    std::cout << "  (k=3 >=55%: answered " << ansR << "/" << accTotal[1][1]
              << " = " << (accTotal[1][1]>0?ansR*100/accTotal[1][1]:0) << "% coverage)" << std::endl;
    return 0;
}

// Export ONE map as its (x,y,z) CATEGORY TENSOR: per-layer grid of category indices
// (keyed to a "categories" list) — the learning input for map<->tileset structure.
static int mapTensorOne(const QString &mapPath,const QString &outPath)
{
    MapDecoder dec;
    MapDecoder::Result r;
    QString err;
    if(!dec.decode(mapPath,r,err)) { std::cerr << "decode FAIL " << mapPath.toStdString() << ": " << err.toStdString() << std::endl; return 1; }
    std::map<std::string,int> catIdx;
    std::vector<std::string> cats;
    cats.push_back(std::string());            // index 0 = "" (no category)
    catIdx[std::string()]=0;
    QJsonArray layersArr;
    size_t z=0;
    while(z<r.layerGrids.size())
    {
        const std::vector<std::string> &g=r.layerGrids.at(z);
        QJsonArray grid;
        size_t i=0;
        while(i<g.size())
        {
            std::map<std::string,int>::iterator it=catIdx.find(g.at(i));
            int idx;
            if(it==catIdx.end()) { idx=(int)cats.size(); catIdx[g.at(i)]=idx; cats.push_back(g.at(i)); }
            else idx=it->second;
            grid.append(idx);
            i++;
        }
        QJsonObject lo;
        lo["name"]=QString::fromStdString(r.layerNames.at(z));
        lo["z"]=(int)z;
        lo["grid"]=grid;          // row-major, length w*h, value = category index
        layersArr.append(lo);
        z++;
    }
    QJsonArray catsArr;
    size_t c=0;
    while(c<cats.size()) { catsArr.append(QString::fromStdString(cats.at(c))); c++; }
    QJsonObject root;
    root["map"]=QFileInfo(mapPath).fileName();
    root["w"]=r.w; root["h"]=r.h; root["tileW"]=r.tileW; root["tileH"]=r.tileH;
    root["categories"]=catsArr;
    root["layers"]=layersArr;
    QFile f(outPath);
    if(!f.open(QIODevice::WriteOnly)) { std::cerr << "cannot write " << outPath.toStdString() << std::endl; return 1; }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    f.close();
    std::cout << QFileInfo(mapPath).fileName().toStdString() << ": " << r.w << "x" << r.h << "x" << r.layerGrids.size()
              << " tensor (" << (cats.size()-1) << " categories, " << r.tagged << "/" << r.totalDrawn << " tagged) -> "
              << outPath.toStdString() << std::endl;
    return 0;
}

static int runMapTensor(const QStringList &args)
{
    if(args.size()<2) { std::cerr << "maptensor needs: <map.tmx | map-dir> <out.json | out-dir>" << std::endl; return 1; }
    QFileInfo fi(args.at(0));
    if(fi.isDir())
    {
        QDir outDir(args.at(1));
        if(!outDir.exists()) QDir().mkpath(args.at(1));
        QDirIterator it(args.at(0),QStringList()<<"*.tmx",QDir::Files,QDirIterator::Subdirectories);
        int n=0,rc=0;
        while(it.hasNext())
        {
            const QString tmx=it.next();
            const QString out=outDir.absoluteFilePath(QFileInfo(tmx).completeBaseName()+".json");
            if(mapTensorOne(tmx,out)!=0) rc=1; else n++;
        }
        std::cout << "exported " << n << " map tensor(s) -> " << args.at(1).toStdString() << std::endl;
        return rc;
    }
    return mapTensorOne(args.at(0),args.at(1));
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

// Place N rectangles of a zone category, sizes sampled from the learned dist.
static void placeZone(std::vector<std::string> &grid,std::vector<char> &occ,int W,int H,
                      const std::map<std::string,std::map<std::string,long> > &zoneSizes,
                      const std::string &cat,int n,std::mt19937 &rng)
{
    std::map<std::string,std::map<std::string,long> >::const_iterator zit=zoneSizes.find(cat);
    if(zit==zoneSizes.cend())
        return;
    int placed=0,tries=0;
    while(placed<n && tries<n*25)
    {
        int w=2,h=2;
        parseWxH(sampleSize(zit->second,rng),w,h);
        if(w>=1 && h>=1 && w<=W-4 && h<=H-4)
        {
            std::uniform_int_distribution<int> dx(2,W-2-w),dy(2,H-2-h);
            const int x=dx(rng),y=dy(rng);
            if(rectFree(occ,W,H,x,y,w,h)) { fillRect(grid,occ,W,x,y,w,h,cat); placed++; }
        }
        tries++;
    }
}

// Synthesise a category grid STRUCTURE-FIRST: walkable ground base, a path cross,
// then rigid rectangular water / tall-grass / building / tree features.
static std::vector<std::string> generateGrid(const std::map<std::string,long> &buildingSizes,
                                             const std::map<std::string,std::map<std::string,long> > &zoneSizes,
                                             int W,int H,unsigned seed)
{
    std::mt19937 rng(seed);
    const std::string base = zoneSizes.count("grass-short")>0 ? "grass-short" : "ground";
    const std::string path = zoneSizes.count("path")>0 ? "path" : base;
    std::vector<std::string> grid(W*H,base);
    std::vector<char> occ(W*H,0);

    // tree border (the enclosed overworld look)
    {
        int y=0;
        while(y<H) { int x=0; while(x<W) { if(x<2||y<2||x>=W-2||y>=H-2) { grid[x+y*W]="tree-trunk"; occ[x+y*W]=1; } x++; } y++; }
    }

    // a path cross gives the town a road structure (kept clear so it stays walkable)
    {
        const int ry=H/2,rx=W/2;
        int x=2;
        while(x<W-2) { grid[x+ry*W]=path; grid[x+(ry+1)*W]=path; occ[x+ry*W]=1; occ[x+(ry+1)*W]=1; x++; }
        int y=2;
        while(y<H-2) { grid[rx+y*W]=path; grid[(rx+1)+y*W]=path; occ[rx+y*W]=1; occ[(rx+1)+y*W]=1; y++; }
    }

    placeZone(grid,occ,W,H,zoneSizes,"water",(W*H)/700+1,rng);
    placeZone(grid,occ,W,H,zoneSizes,"grass-tall",(W*H)/250+1,rng);

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
                    int i=0; while(i<w) { grid[(x+i)+y*W]="building-roof"; i++; }
                    grid[(x+w/2)+(y+h-1)*W]="door";
                    placed++;
                }
            }
            tries++;
        }
    }

    placeZone(grid,occ,W,H,zoneSizes,"tree-trunk",(W*H)/300+1,rng);
    return grid;
}

// category -> the CatchChallenger engine layer the tile is placed on.
static std::string categoryToLayer(const std::string &cat)
{
    if(cat=="water" || cat=="water-edge" || cat=="waterfall" || cat=="lava") return "Water";
    if(cat=="grass-tall") return "Grass";
    if(cat=="ledge-down") return "LedgesBottom";
    if(cat=="ledge-up") return "LedgesTop";
    if(cat=="ledge-left") return "LedgesLeft";
    if(cat=="ledge-right") return "LedgesRight";
    if(cat=="tree-canopy" || cat=="building-roof") return "WalkBehind";
    if(cat=="building-wall" || cat=="cliff" || cat=="rock" || cat=="tree-trunk"
       || cat=="fence" || cat=="sign" || cat=="bush" || cat=="window") return "Collisions";
    return "Walkable";   // ground/path/sand/grass-short/door/stairs/flower/interior-*
}

// AUTO-TILING context: per (tileset .tsx canonical path, tileId) -> for each of the
// 4 directions (N,E,S,W) the count of each neighbour CATEGORY, learned from the
// maps. A grass tile usually seen with water to the east scores high where the
// generated grid has water to the east -> the right shore/edge tile is chosen.
typedef std::map<std::pair<std::string,int>,std::vector<std::map<std::string,long> > > ContextModel;

static void learnContext(const QString &mapsDir,ContextModel &ctx)
{
    MapDecoder dec;
    QDirIterator it(mapsDir,QStringList()<<"*.tmx",QDir::Files,QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        MapDecoder::Result r;
        QString err;
        if(!dec.decode(it.next(),r,err))
            continue;
        const int w=r.w,h=r.h;
        int y=0;
        while(y<h)
        {
            int x=0;
            while(x<w)
            {
                const std::string &canon=r.topTilesetCanon.at(x+y*w);
                const int id=r.topTileId.at(x+y*w);
                if(!canon.empty() && id>=0)
                {
                    std::vector<std::map<std::string,long> > &v=ctx[std::make_pair(canon,id)];
                    if(v.empty())
                        v.resize(4);
                    if(y>0)   { const std::string &n=r.categoryGrid.at(x+(y-1)*w); if(!n.empty()) v[0][n]++; }
                    if(x+1<w) { const std::string &n=r.categoryGrid.at((x+1)+y*w); if(!n.empty()) v[1][n]++; }
                    if(y+1<h) { const std::string &n=r.categoryGrid.at(x+(y+1)*w); if(!n.empty()) v[2][n]++; }
                    if(x>0)   { const std::string &n=r.categoryGrid.at((x-1)+y*w); if(!n.empty()) v[3][n]++; }
                }
                x++;
            }
            y++;
        }
    }
}

static double contextScore(const ContextModel &ctx,const std::pair<std::string,int> &key,const std::string nb[4])
{
    ContextModel::const_iterator it=ctx.find(key);
    if(it==ctx.cend())
        return -1.0;   // tile never observed -> untrained
    double s=0.0;
    int d=0;
    while(d<4)
    {
        const std::map<std::string,long> &dm=it->second.at(d);
        long tot=0;
        std::map<std::string,long>::const_iterator m=dm.begin();
        while(m!=dm.cend()) { tot+=m->second; ++m; }
        if(tot>0)
        {
            std::map<std::string,long>::const_iterator f=dm.find(nb[d]);
            const long match = f!=dm.cend() ? f->second : 0;
            s += (double)match/(double)tot;
        }
        d++;
    }
    return s;
}

// Fraction of a tile that is substantially opaque (alpha>=128).  Used to reject
// transparent fragments a noisy heuristic may have mis-tagged (e.g. building edges
// tagged "cliff") so the generator never places an invisible tile.
static double tileOpaqueFrac(TagModel *tm,int id)
{
    const QImage &img=tm->image();
    if(img.isNull() || tm->columns()<1 || tm->tileWidth()<1 || tm->tileHeight()<1)
        return 0.0;
    const int tw=tm->tileWidth(), th=tm->tileHeight();
    const int x0=(id%tm->columns())*tw, y0=(id/tm->columns())*th;
    if(x0+tw>img.width() || y0+th>img.height())
        return 0.0;
    const QImage c=img.convertToFormat(QImage::Format_ARGB32);
    long opaque=0,total=0;
    int y=0;
    while(y<th)
    {
        const QRgb *line=reinterpret_cast<const QRgb*>(c.scanLine(y0+y));
        int x=0;
        while(x<tw) { if(qAlpha(line[x0+x])>=128) opaque++; total++; x++; }
        y++;
    }
    return total>0 ? (double)opaque/(double)total : 0.0;
}

// Turn a CATEGORY grid (one category string per cell, "" = empty) into a valid
// engine .tmx on the target tileset: resolve each category to its REAL tagged
// tiles, auto-tile by learned neighbour context, distribute into the named engine
// layers, write CSV layers + relative tileset refs.  Shared by the struct-based
// (--genmap) and the coherent WFC (--genwfc) front-ends.
static int writeTmxFromGrid(const std::vector<std::string> &grid,int W,int H,
                            const QString &tilesetDir,const QString &outPath,unsigned seed,
                            const std::vector<std::pair<int,int> > &bots=std::vector<std::pair<int,int> >());

static int runGenMap(const QStringList &args)
{
    // <struct.json> <tileset-dir> <out.tmx> [W H seed]
    if(args.size()<3) { std::cerr << "genmap needs: <struct.json> <tileset-dir> <out.tmx> [W H seed]" << std::endl; return 1; }
    std::map<std::string,long> buildingSizes;
    std::map<std::string,std::map<std::string,long> > zoneSizes;
    if(!loadStruct(args.at(0),buildingSizes,zoneSizes)) { std::cerr << "cannot load " << args.at(0).toStdString() << std::endl; return 1; }
    const int W = args.size()>3 ? args.at(3).toInt() : 40;
    const int H = args.size()>4 ? args.at(4).toInt() : 30;
    const unsigned seed = args.size()>5 ? (unsigned)args.at(5).toUInt() : 1u;
    const std::vector<std::string> grid=generateGrid(buildingSizes,zoneSizes,W,H,seed);
    return writeTmxFromGrid(grid,W,H,args.at(1),args.at(2),seed);
}
static int writeTmxFromGrid(const std::vector<std::string> &grid,int W,int H,
                            const QString &tilesetDir,const QString &outPath,unsigned seed,
                            const std::vector<std::pair<int,int> > &bots)
{
    // load the official tilesets + their sidecar tags; category -> first (tileset,tileId)
    QDir tdir(tilesetDir);
    const QStringList tsxs=tdir.entryList(QStringList()<<"*.tsx",QDir::Files,QDir::Name);
    std::vector<TagModel*> models;
    std::vector<int> tileCounts;
    std::vector<QString> tsxAbs;
    // category -> CANDIDATE tiles (usage, (modelIdx, tileId)). We keep the top few
    // most-used tiles per category and sample among them per cell for TEXTURE.
    std::map<std::string,std::vector<std::pair<long,std::pair<int,int> > > > catCand;
    int ti=0;
    while(ti<tsxs.size())
    {
        TagModel *tm=new TagModel();
        const QString abs=tdir.absoluteFilePath(tsxs.at(ti));
        if(tm->load(abs))
        {
            const int idx=(int)models.size();
            models.push_back(tm);
            tileCounts.push_back(tm->tileCount());
            tsxAbs.push_back(abs);
            MapUsageIndex ui;
            ui.build(abs);
            const std::map<int,MapUsageIndex::TileStat> stats=ui.analyzeAllTiles();
            int id=0;
            while(id<tm->tileCount())
            {
                const std::string &cat=tm->tagOf(id).category;
                if(!cat.empty() && tileOpaqueFrac(tm,id)>=0.5)   // skip transparent fragments (mis-tagged building edges etc.)
                {
                    std::map<int,MapUsageIndex::TileStat>::const_iterator s=stats.find(id);
                    const long use = s!=stats.cend() ? s->second.total : 0;
                    catCand[cat].push_back(std::make_pair(use,std::make_pair(idx,id)));
                }
                id++;
            }
        }
        else
            delete tm;
        ti++;
    }
    // keep the top 24 most-used tiles per category (enough to include edge/variant
    // tiles for the auto-tiler), with a parallel usage list for tie-breaking.
    std::map<std::string,std::vector<std::pair<int,int> > > catTiles;
    std::map<std::string,std::vector<long> > catUsage;
    {
        std::map<std::string,std::vector<std::pair<long,std::pair<int,int> > > >::iterator c=catCand.begin();
        while(c!=catCand.end())
        {
            std::sort(c->second.rbegin(),c->second.rend());
            size_t k=0;
            while(k<c->second.size() && k<24) { catTiles[c->first].push_back(c->second.at(k).second); catUsage[c->first].push_back(c->second.at(k).first); k++; }
            ++c;
        }
    }
    // a few model categories have no exact target equivalent (the target splits them
    // finer, or lacks them) -> alias to the closest present category so those cells
    // get a real tile instead of a hole.  ALIAS{generic -> {fallbacks}}.
    {
        const char *aliases[4][4]={
            {"terrain","ground","grass-short","path"},     // generic fill -> walkable ground
            {"building","building-wall","cliff","rock"},    // whole building -> its wall
            {"bush","grass-tall","grass-short","tree-trunk"},
            {"flower","grass-short","ground","path"}};
        int a=0;
        while(a<4)
        {
            if(catTiles.find(aliases[a][0])==catTiles.cend())
            {
                int gi=1;
                while(gi<4)
                {
                    std::map<std::string,std::vector<std::pair<int,int> > >::iterator it=catTiles.find(aliases[a][gi]);
                    if(it!=catTiles.cend()) { catTiles[aliases[a][0]]=it->second; catUsage[aliases[a][0]]=catUsage[aliases[a][gi]]; break; }
                    gi++;
                }
            }
            a++;
        }
    }

    // categories the grid uses; which are missing from the official set
    std::set<std::string> used;
    size_t g=0;
    while(g<grid.size()) { if(!grid.at(g).empty()) used.insert(grid.at(g)); g++; }
    std::vector<std::string> missing;
    std::set<std::string>::const_iterator u=used.begin();
    while(u!=used.cend()) { if(catTiles.find(*u)==catTiles.cend()) missing.push_back(*u); ++u; }

    // firstgids for the tilesets used by any kept tile
    std::set<int> usedTs;
    {
        std::map<std::string,std::vector<std::pair<int,int> > >::const_iterator c=catTiles.begin();
        while(c!=catTiles.cend())
        {
            if(used.count(c->first)>0) { size_t k=0; while(k<c->second.size()) { usedTs.insert(c->second.at(k).first); k++; } }
            ++c;
        }
    }
    // bot markers come from invisible.tsx (tile 0) — ensure it gets a firstgid + ref
    int invisibleIdx=-1;
    if(!bots.empty())
    {
        size_t ii=0;
        while(ii<tsxAbs.size()) { if(QFileInfo(tsxAbs.at(ii)).fileName()=="invisible.tsx") { invisibleIdx=(int)ii; break; } ii++; }
        if(invisibleIdx>=0) usedTs.insert(invisibleIdx);
    }
    std::map<int,int> firstgid;
    int run=1;
    std::set<int>::const_iterator ts=usedTs.begin();
    while(ts!=usedTs.cend()) { firstgid[*ts]=run; run+=tileCounts.at(*ts); ++ts; }
    std::map<std::string,std::vector<int> > catGids;
    {
        std::map<std::string,std::vector<std::pair<int,int> > >::const_iterator c=catTiles.begin();
        while(c!=catTiles.cend())
        {
            size_t k=0;
            while(k<c->second.size()) { const std::pair<int,int> &t=c->second.at(k); if(usedTs.count(t.first)>0) catGids[c->first].push_back(firstgid[t.first]+t.second); k++; }
            ++c;
        }
    }

    // canonical .tsx path per loaded tileset (for context lookup)
    std::vector<std::string> canonOf(tsxAbs.size());
    {
        size_t i=0;
        while(i<tsxAbs.size()) { canonOf[i]=QFileInfo(tsxAbs.at(i)).canonicalFilePath().toStdString(); i++; }
    }
    // learn the auto-tiling context from the maps near the tileset dir (its parent
    // holds main/ for the official set, johto//kanto/ for the model set).
    ContextModel ctx;
    learnContext(QDir(tilesetDir).absoluteFilePath(".."),ctx);

    // distribute gids into per-layer grids; AUTO-TILE by picking, per cell, the
    // candidate whose learned 4-neighbour context best matches the generated
    // neighbourhood (tie -> the more-used tile).
    std::map<std::string,std::vector<int> > layers;
    int y=0;
    while(y<H)
    {
        int x=0;
        while(x<W)
        {
            const std::string &cat=grid.at(x+y*W);
            std::map<std::string,std::vector<int> >::const_iterator gi=catGids.find(cat);
            if(!cat.empty() && gi!=catGids.cend() && !gi->second.empty())
            {
                const std::vector<int> &gids=gi->second;
                const std::vector<std::pair<int,int> > &cands=catTiles[cat];
                const std::vector<long> &uses=catUsage[cat];
                std::string nb[4];
                nb[0] = y>0   ? grid.at(x+(y-1)*W) : std::string();
                nb[1] = x+1<W ? grid.at((x+1)+y*W) : std::string();
                nb[2] = y+1<H ? grid.at(x+(y+1)*W) : std::string();
                nb[3] = x>0   ? grid.at((x-1)+y*W) : std::string();
                int pick=0;
                double bestScore=-2.0;
                long bestUse=-1;
                size_t k=0;
                while(k<cands.size() && k<gids.size())
                {
                    const double sc=contextScore(ctx,std::make_pair(canonOf.at(cands.at(k).first),cands.at(k).second),nb);
                    const long use = k<uses.size() ? uses.at(k) : 0;
                    if(sc>bestScore || (sc==bestScore && use>bestUse)) { bestScore=sc; bestUse=use; pick=(int)k; }
                    k++;
                }
                const std::string ln=categoryToLayer(cat);
                if(layers.find(ln)==layers.cend())
                    layers[ln]=std::vector<int>(W*H,0);
                layers[ln][x+y*W]=gids.at(pick);
            }
            x++;
        }
        y++;
    }

    // write the .tmx (CSV layers, relative tileset refs) into map/main/generated/
    const QString outDir=QFileInfo(outPath).absolutePath();
    QDir().mkpath(outDir);
    QFile out(outPath);
    if(!out.open(QIODevice::WriteOnly|QIODevice::Truncate)) { std::cerr << "cannot write " << outPath.toStdString() << std::endl; return 1; }
    QTextStream w(&out);
    w << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    w << "<map version=\"1.10\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\"" << W
      << "\" height=\"" << H << "\" tilewidth=\"16\" tileheight=\"16\" infinite=\"0\" nextlayerid=\"99\" nextobjectid=\"1\">\n";
    ts=usedTs.begin();
    while(ts!=usedTs.cend())
    {
        const QString rel=QDir(outDir).relativeFilePath(tsxAbs.at(*ts));
        w << " <tileset firstgid=\"" << firstgid[*ts] << "\" source=\"" << rel << "\"/>\n";
        ++ts;
    }
    const char *order[9]={"Walkable","Grass","Water","LedgesBottom","LedgesTop","LedgesLeft","LedgesRight","Collisions","WalkBehind"};
    int lid=1,oi=0;
    while(oi<9)
    {
        std::map<std::string,std::vector<int> >::const_iterator lit=layers.find(order[oi]);
        if(lit!=layers.cend())
        {
            w << " <layer id=\"" << lid << "\" name=\"" << order[oi] << "\" width=\"" << W << "\" height=\"" << H << "\">\n";
            w << "  <data encoding=\"csv\">\n";
            int yy=0;
            while(yy<H)
            {
                int xx=0;
                while(xx<W) { w << lit->second.at(xx+yy*W); if(!(yy==H-1 && xx==W-1)) w << ","; xx++; }
                w << "\n";
                yy++;
            }
            w << "</data>\n </layer>\n";
            lid++;
        }
        oi++;
    }
    // bots: the Object group with invisible.tsx markers (type="bot" + id property,
    // matching the sibling <map>.xml the caller writes).
    if(!bots.empty() && invisibleIdx>=0)
    {
        w << " <objectgroup id=\"90\" name=\"Object\">\n";
        size_t b=0;
        while(b<bots.size())
        {
            const int bx=bots.at(b).first, by=bots.at(b).second;
            w << "  <object id=\"" << (b+1) << "\" type=\"bot\" gid=\"" << firstgid[invisibleIdx]
              << "\" x=\"" << bx*16 << "\" y=\"" << (by+1)*16 << "\" width=\"16\" height=\"16\">\n";
            w << "   <properties><property name=\"id\" type=\"int\" value=\"" << (b+1) << "\"/></properties>\n";
            w << "  </object>\n";
            b++;
        }
        w << " </objectgroup>\n";
    }
    w << "</map>\n";
    out.close();

    size_t m=0;
    while(m<models.size()) { delete models.at(m); m++; }

    std::cout << "genmap " << W << "x" << H << " (seed " << seed << ") -> " << outPath.toStdString()
              << "  using " << usedTs.size() << " tileset(s), " << catGids.size() << " categories mapped" << std::endl;
    if(!missing.empty())
    {
        std::cout << "  MISSING from the official tileset (add tiles + tag these): ";
        size_t i=0;
        while(i<missing.size()) { std::cout << missing.at(i) << " "; i++; }
        std::cout << std::endl;
    }
    return 0;
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
    const std::vector<std::string> grid=generateGrid(buildingSizes,zoneSizes,W,H,seed);

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
        // recurse: a datapack has per-label/region tileset subfolders
        QStringList tsxs;
        QDirIterator it(args.at(0),QStringList()<<"*.tsx",QDir::Files,QDirIterator::Subdirectories);
        while(it.hasNext()) tsxs.append(it.next());
        tsxs.sort();
        std::cout << "suggest: " << tsxs.size() << " tileset(s) under " << args.at(0).toStdString() << std::endl;
        int rc=0,k=0;
        while(k<tsxs.size()) { if(suggestOne(tsxs.at(k))!=0) rc=1; k++; }
        std::cout << "done — open each in the GUI to set the visually-ambiguous categories." << std::endl;
        return rc;
    }
    return suggestOne(args.at(0));
}

// Recursively collect tile layers (into groups too).
static void usedCollectTL(const QList<Tiled::Layer*> &ls,std::vector<Tiled::TileLayer*> &out)
{
    int i=0;
    while(i<ls.size())
    {
        Tiled::Layer *l=ls.at(i);
        if(l->isTileLayer())
            out.push_back(static_cast<Tiled::TileLayer*>(l));
        else if(l->isGroupLayer())
            usedCollectTL(static_cast<Tiled::GroupLayer*>(l)->layers(),out);
        i++;
    }
}
// Scan EVERY .tmx under map-dir and record, per tileset (canonical .tsx path), the
// set of tile ids actually placed on a map.  -> out.json { "<abs.tsx>": [ids...] }.
static int runUsedTiles(const QStringList &args)
{
    if(args.size()<2) { std::cerr << "usedtiles needs: <map-dir> <out.json>" << std::endl; return 1; }
    std::map<std::string,std::set<int> > used;
    QDirIterator it(args.at(0),QStringList()<<"*.tmx",QDir::Files,QDirIterator::Subdirectories);
    int nmaps=0;
    while(it.hasNext())
    {
        const QString path=it.next();
        Tiled::MapReader reader;
        std::unique_ptr<Tiled::Map> m=reader.readMap(path);
        if(m==nullptr)
        {
            std::cerr << "  WARN cannot read " << path.toStdString() << ": " << reader.errorString().toStdString() << std::endl;
            continue;
        }
        nmaps++;
        std::vector<Tiled::TileLayer*> layers;
        usedCollectTL(m->layers(),layers);
        size_t li=0;
        while(li<layers.size())
        {
            Tiled::TileLayer *tl=layers.at(li);
            int y=0;
            while(y<tl->height())
            {
                int x=0;
                while(x<tl->width())
                {
                    const Tiled::Cell &c=tl->cellAt(x,y);
                    if(!c.isEmpty() && c.tileset()!=nullptr)
                        used[QFileInfo(c.tileset()->fileName()).canonicalFilePath().toStdString()].insert(c.tileId());
                    x++;
                }
                y++;
            }
            li++;
        }
    }
    QJsonObject root;
    std::map<std::string,std::set<int> >::iterator u=used.begin();
    while(u!=used.end())
    {
        QJsonArray arr;
        std::set<int>::iterator s=u->second.begin();
        while(s!=u->second.end()) { arr.append(*s); ++s; }
        root[QString::fromStdString(u->first)]=arr;
        ++u;
    }
    QFile f(args.at(1));
    if(!f.open(QIODevice::WriteOnly)) { std::cerr << "cannot write " << args.at(1).toStdString() << std::endl; return 1; }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    f.close();
    std::cout << "scanned " << nmaps << " map(s); " << used.size() << " tileset(s) used -> " << args.at(1).toStdString() << std::endl;
    return 0;
}

// LEARN the category -> engine-LAYER mapping from the model maps (the input the
// category-grid -> .tmx bridge needs to decompose a generated category grid into
// the engine's named layers).  For every tagged cell, count which layer NAME it is
// drawn on; the dominant layer is the category's canonical layer, and the engine's
// walkability precedence (Collisions/WalkBehind = blocked/above-player, the rest
// walkable) then follows.  Pure analysis — emits no datapack assets.
static void catLayersSortDesc(std::vector<std::pair<long,std::string> > &v)
{
    size_t a=1;
    while(a<v.size())
    {
        const std::pair<long,std::string> key=v[a];
        size_t b=a;
        while(b>0 && v[b-1].first<key.first) { v[b]=v[b-1]; b--; }
        v[b]=key;
        a++;
    }
}
static int runCatLayers(const QStringList &args)
{
    if(args.isEmpty()) { std::cerr << "catlayers needs: <model-map-dir> [out.json]" << std::endl; return 1; }
    std::map<std::string,std::map<std::string,long> > catLayer;   // category -> layerName -> count
    std::map<std::string,long> catTotal;
    int nmaps=0;
    QDirIterator it(args.at(0),QStringList()<<"*.tmx",QDir::Files,QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        MapDecoder dec;
        MapDecoder::Result r;
        QString err;
        if(dec.decode(it.next(),r,err))
        {
            nmaps++;
            size_t z=0;
            while(z<r.layerGrids.size())
            {
                const std::string &ln=r.layerNames.at(z);
                const std::vector<std::string> &g=r.layerGrids.at(z);
                size_t i=0;
                while(i<g.size()) { if(!g[i].empty()) { catLayer[g[i]][ln]++; catTotal[g[i]]++; } i++; }
                z++;
            }
        }
    }
    if(nmaps==0) { std::cerr << "no maps decoded in " << args.at(0).toStdString() << std::endl; return 1; }
    // order categories by total cell count, desc
    std::vector<std::pair<long,std::string> > order;
    std::map<std::string,long>::iterator ct=catTotal.begin();
    while(ct!=catTotal.end()) { order.push_back(std::pair<long,std::string>(ct->second,ct->first)); ++ct; }
    catLayersSortDesc(order);
    QJsonObject out;
    std::cout << "category -> engine layer (" << nmaps << " maps, dominant layer = canonical):" << std::endl;
    size_t o=0;
    while(o<order.size())
    {
        const std::string &cat=order[o].second;
        const long total=order[o].first;
        // sort this category's layers desc
        std::vector<std::pair<long,std::string> > ls;
        std::map<std::string,long>::iterator l=catLayer[cat].begin();
        while(l!=catLayer[cat].end()) { ls.push_back(std::pair<long,std::string>(l->second,l->first)); ++l; }
        catLayersSortDesc(ls);
        std::string line;
        size_t k=0;
        while(k<ls.size() && k<3)
        {
            const int pct=(int)(ls[k].first*100/total);
            if(!line.empty()) line+=", ";
            line+=ls[k].second+" "+std::to_string(pct)+"%";
            k++;
        }
        char buf[24]; std::snprintf(buf,sizeof(buf),"%-18s",cat.c_str());
        std::cout << "  " << buf << " " << total << "\t" << line << std::endl;
        out[QString::fromStdString(cat)]=QString::fromStdString(ls.empty()?std::string():ls[0].second);
        o++;
    }
    if(args.size()>1)
    {
        QFile f(args.at(1));
        if(f.open(QIODevice::WriteOnly)) { f.write(QJsonDocument(out).toJson(QJsonDocument::Indented)); f.close();
            std::cout << "canonical category->layer map -> " << args.at(1).toStdString() << std::endl; }
    }
    return 0;
}

// --- Wave Function Collapse (example-based, coherent-by-construction) ----------
// Learns 4-direction category adjacency from the MODEL maps, then generates a new
// category grid where every neighbour pair is one the model actually used.  An
// ALTERNATIVE to the structure-first generator, to compare.
static int wfcCatIndex(std::map<std::string,int> &m,std::vector<std::string> &v,const std::string &c)
{
    std::map<std::string,int>::iterator it=m.find(c);
    if(it!=m.end())
        return it->second;
    const int id=(int)v.size();
    m[c]=id;
    v.push_back(c);
    return id;
}
// Restrict neighbour domain ndom to categories b reachable from srcdom via adj[a][b].
static bool wfcRestrict(const std::vector<char> &srcdom,std::vector<char> &ndom,int &nrem,
                        const std::vector<std::vector<char> > &adj,int K,bool &contradiction)
{
    bool changed=false;
    int b=0;
    while(b<K)
    {
        if(ndom[b])
        {
            bool ok=false;
            int a=0;
            while(a<K) { if(srcdom[a] && adj[a][b]) { ok=true; break; } a++; }
            if(!ok) { ndom[b]=0; nrem--; changed=true; }
        }
        b++;
    }
    if(nrem==0)
        contradiction=true;
    return changed;
}
// Shared model loader for the WFC variants: decode every model .tmx to its
// TOPMOST-category grid, each category string mapped to a small int id.
struct WfcGrid { int w; int h; std::vector<int> c; };
static void wfcLoadModel(const QString &dir,std::map<std::string,int> &catId,
                         std::vector<std::string> &cats,std::vector<WfcGrid> &gs)
{
    wfcCatIndex(catId,cats,std::string());   // 0 = empty/none
    QDirIterator dit(dir,QStringList()<<"*.tmx",QDir::Files,QDirIterator::Subdirectories);
    while(dit.hasNext())
    {
        MapDecoder dec;
        MapDecoder::Result r;
        QString err;
        if(dec.decode(dit.next(),r,err))
        {
            WfcGrid g; g.w=r.w; g.h=r.h; g.c.resize(r.w*r.h);
            int i=0;
            while(i<r.w*r.h) { g.c[i]=wfcCatIndex(catId,cats,r.categoryGrid[i]); i++; }
            gs.push_back(g);
        }
    }
}
// The set of 2x2 category blocks the model actually uses — the coherence yardstick.
static void wfcBuild2x2(const std::vector<WfcGrid> &gs,std::set<std::vector<int> > &out)
{
    size_t gi=0;
    while(gi<gs.size())
    {
        const WfcGrid &g=gs.at(gi);
        int y=0;
        while(y+1<g.h)
        {
            int x=0;
            while(x+1<g.w)
            {
                std::vector<int> b(4);
                b[0]=g.c[x+y*g.w]; b[1]=g.c[(x+1)+y*g.w];
                b[2]=g.c[x+(y+1)*g.w]; b[3]=g.c[(x+1)+(y+1)*g.w];
                out.insert(b);
                x++;
            }
            y++;
        }
        gi++;
    }
}
// Fraction (%) of the WxH result's 2x2 windows that exist in the model set —
// higher = more coherent (fewer neighbour pairings the model never used).
static double wfcCoherence(const std::vector<int> &grid,int W,int H,const std::set<std::vector<int> > &model2x2)
{
    long tot=0,ok=0;
    int y=0;
    while(y+1<H)
    {
        int x=0;
        while(x+1<W)
        {
            std::vector<int> b(4);
            b[0]=grid[x+y*W]; b[1]=grid[(x+1)+y*W];
            b[2]=grid[x+(y+1)*W]; b[3]=grid[(x+1)+(y+1)*W];
            tot++;
            if(model2x2.find(b)!=model2x2.cend()) ok++;
            x++;
        }
        y++;
    }
    return tot>0?(double)ok*100.0/(double)tot:0.0;
}
static int runWfc(const QStringList &args)
{
    if(args.size()<2) { std::cerr << "wfc needs: <model-map-dir> <out.png> [W H seed]" << std::endl; return 1; }
    const int W = args.size()>2 ? args.at(2).toInt() : 30;
    const int H = args.size()>3 ? args.at(3).toInt() : 30;
    const unsigned int seed = args.size()>4 ? args.at(4).toUInt() : 1234u;
    const int cohesion = args.size()>5 ? args.at(5).toInt() : 1;   // >1 = favour the left/up neighbour's category (bigger blocks)
    // LEARN from the model maps' TOPMOST-category grids
    std::map<std::string,int> catId;
    std::vector<std::string> cats;
    std::vector<WfcGrid> gs;
    wfcLoadModel(args.at(0),catId,cats,gs);
    const int K=(int)cats.size();
    if(K<2) { std::cerr << "WFC: no tagged categories in " << args.at(0).toStdString() << std::endl; return 1; }
    std::vector<std::vector<char> > rightOK(K,std::vector<char>(K,0)),downOK(K,std::vector<char>(K,0));
    std::vector<std::vector<char> > leftOK(K,std::vector<char>(K,0)),upOK(K,std::vector<char>(K,0));
    std::vector<long> freq(K,0);
    size_t gi=0;
    while(gi<gs.size())
    {
        const WfcGrid &g=gs.at(gi);
        int y=0;
        while(y<g.h)
        {
            int x=0;
            while(x<g.w)
            {
                const int a=g.c[x+y*g.w];
                freq[a]++;
                if(x+1<g.w) { const int b=g.c[(x+1)+y*g.w]; rightOK[a][b]=1; leftOK[b][a]=1; }
                if(y+1<g.h) { const int b=g.c[x+(y+1)*g.w]; downOK[a][b]=1; upOK[b][a]=1; }
                x++;
            }
            y++;
        }
        gi++;
    }
    // GENERATE WxH, restart on contradiction
    std::vector<int> result(W*H,0);
    bool done=false;
    int attempt=0;
    while(attempt<60 && !done)
    {
        std::mt19937 rng(seed+(unsigned int)attempt);
        std::vector<std::vector<char> > dom(W*H,std::vector<char>(K,1));
        std::vector<int> rem(W*H,K);
        bool contradiction=false;
        while(!contradiction)
        {
            // lowest-entropy uncollapsed cell
            int best=-1,bestRem=K+1;
            int c=0;
            while(c<W*H) { if(rem[c]>1 && rem[c]<bestRem) { bestRem=rem[c]; best=c; } c++; }
            if(best<0)
                break;   // all cells decided
            // collapse: pick a possible category weighted by frequency
            // cohesion: boost categories matching the already-placed left/up neighbour
            int lc=-1,uc=-1;
            if(best%W>0 && rem[best-1]==1) { int z=0; while(z<K) { if(dom[best-1][z]) { lc=z; break; } z++; } }
            if(best/W>0 && rem[best-W]==1) { int z=0; while(z<K) { if(dom[best-W][z]) { uc=z; break; } z++; } }
            long tot=0;
            int k=0;
            while(k<K) { if(dom[best][k]) { long w=freq[k]+1; if(k==lc) w*=cohesion; if(k==uc) w*=cohesion; tot+=w; } k++; }
            long pick=(long)(rng()%(unsigned long)(tot>0?tot:1));
            int chosen=-1;
            k=0;
            while(k<K) { if(dom[best][k]) { long w=freq[k]+1; if(k==lc) w*=cohesion; if(k==uc) w*=cohesion; pick-=w; if(pick<0) { chosen=k; break; } } k++; }
            if(chosen<0) { contradiction=true; break; }
            k=0;
            while(k<K) { dom[best][k]=(k==chosen)?1:0; k++; }
            rem[best]=1;
            // propagate
            std::vector<int> work;
            work.push_back(best);
            while(!work.empty() && !contradiction)
            {
                const int cur=work.back();
                work.pop_back();
                const int cx=cur%W,cy=cur/W;
                if(cx+1<W && wfcRestrict(dom[cur],dom[cur+1],rem[cur+1],rightOK,K,contradiction)) work.push_back(cur+1);
                if(!contradiction && cx>0   && wfcRestrict(dom[cur],dom[cur-1],rem[cur-1],leftOK,K,contradiction)) work.push_back(cur-1);
                if(!contradiction && cy+1<H && wfcRestrict(dom[cur],dom[cur+W],rem[cur+W],downOK,K,contradiction)) work.push_back(cur+W);
                if(!contradiction && cy>0   && wfcRestrict(dom[cur],dom[cur-W],rem[cur-W],upOK,K,contradiction))   work.push_back(cur-W);
            }
        }
        if(!contradiction)
        {
            bool ok=true;
            int c=0;
            while(c<W*H) { int sel=-1,k=0; while(k<K) { if(dom[c][k]) { sel=k; break; } k++; } if(sel<0) { ok=false; break; } result[c]=sel; c++; }
            if(ok)
                done=true;
        }
        attempt++;
    }
    if(!done) { std::cerr << "WFC: no solution after 60 attempts (adjacency too constrained)" << std::endl; return 1; }
    std::set<std::vector<int> > model2x2;
    wfcBuild2x2(gs,model2x2);
    const double coh=wfcCoherence(result,W,H,model2x2);
    QImage img(W*16,H*16,QImage::Format_ARGB32);
    img.fill(QColor(28,28,38));
    QPainter p(&img);
    int yy=0;
    while(yy<H)
    {
        int xx=0;
        while(xx<W)
        {
            const std::string &cat=cats.at(result[xx+yy*W]);
            if(!cat.empty())
                p.fillRect(xx*16,yy*16,16,16,MapDecoder::categoryColor(cat));
            xx++;
        }
        yy++;
    }
    p.end();
    if(!img.save(args.at(1))) { std::cerr << "cannot write " << args.at(1).toStdString() << std::endl; return 1; }
    char cohbuf[32]; std::snprintf(cohbuf,sizeof(cohbuf),"%.1f",coh);
    std::cout << "WFC(single-tile) " << W << "x" << H << " from " << gs.size() << " model map(s), " << (K-1)
              << " categories, attempt " << attempt << ", coherence " << cohbuf << "% (2x2 windows seen in model) -> "
              << args.at(1).toStdString() << std::endl;
    return 0;
}

// --- Overlapping-pattern WFC (the fix for the "mushy" single-tile model) ------
// Learn every NxN category PATTERN the model uses; two patterns may be neighbours
// only if they AGREE on their (N-1)-wide overlap.  Patterns encode local SHAPE, so
// whole buildings / grass blobs reproduce instead of per-cell noise.  AC-4
// propagation (per-cell, per-pattern, per-direction support counters).
static void wfcBanPattern(int cell,int p,int P,
    std::vector<char> &possible,std::vector<int> &nPoss,std::vector<long> &sumW,const std::vector<long> &weight,
    std::vector<int> &compat,std::vector<std::pair<int,int> > &banStack,bool &contradiction)
{
    const size_t base=(size_t)cell*P+p;
    if(!possible[base])
        return;
    possible[base]=0;
    int d=0; while(d<4) { compat[base*4+d]=0; d++; }
    nPoss[cell]--;
    sumW[cell]-=weight[p];
    if(nPoss[cell]==0)
        contradiction=true;
    banStack.push_back(std::pair<int,int>(cell,p));
}
static void wfcPropagate(int P,int W,int H,const std::vector<std::vector<int> > *prop,
    std::vector<char> &possible,std::vector<int> &nPoss,std::vector<long> &sumW,const std::vector<long> &weight,
    std::vector<int> &compat,std::vector<std::pair<int,int> > &banStack,bool &contradiction)
{
    const int opp[4]={1,0,3,2};
    const int ddx[4]={1,-1,0,0};
    const int ddy[4]={0,0,1,-1};
    while(!banStack.empty() && !contradiction)
    {
        const std::pair<int,int> e=banStack.back();
        banStack.pop_back();
        const int cell=e.first, p=e.second;
        const int cx=cell%W, cy=cell/W;
        int d=0;
        while(d<4)
        {
            const int nx=cx+ddx[d], ny=cy+ddy[d];
            if(nx>=0 && ny>=0 && nx<W && ny<H)
            {
                const int ncell=nx+ny*W;
                const std::vector<int> &lst=prop[d][p];   // patterns p supported as its d-neighbour
                size_t i=0;
                while(i<lst.size())
                {
                    const int t=lst[i];
                    int &cc=compat[((size_t)ncell*P+t)*4+opp[d]];   // t at ncell, support from the `cell` side
                    if(cc>0)
                    {
                        cc--;
                        if(cc==0 && possible[(size_t)ncell*P+t])
                            wfcBanPattern(ncell,t,P,possible,nPoss,sumW,weight,compat,banStack,contradiction);
                    }
                    i++;
                }
            }
            d++;
        }
    }
}
// Generate a coherent WxH category-id grid (result, values index cats[]) via the
// overlapping-pattern model.  Fills catId/cats/gs and reports mode ("AC4"/"greedy")
// + pattern count.  Reused by --wfco (PNG) and --genmap (.tmx).
static bool wfcGenerateGrid(const QString &modelDir,int N,int W,int H,unsigned int seed,
                            std::map<std::string,int> &catId,std::vector<std::string> &cats,
                            std::vector<WfcGrid> &gs,std::vector<int> &result,std::string &modeOut,int &Pout)
{
    if(gs.empty())   // reuse a pre-decoded model across many generated maps
        wfcLoadModel(modelDir,catId,cats,gs);
    const int K=(int)cats.size();
    if(K<2) { std::cerr << "WFCo: no tagged categories in " << modelDir.toStdString() << std::endl; return false; }
    if(K>255) { std::cerr << "WFCo: " << K << " categories > 255 (byte signatures)" << std::endl; return false; }

    // 1) extract NxN patterns + weights
    std::map<std::vector<int>,int> patIndex;
    std::vector<std::vector<int> > pats;
    std::vector<long> weight;
    size_t gi=0;
    while(gi<gs.size())
    {
        const WfcGrid &g=gs.at(gi);
        int y=0;
        while(y+N<=g.h)
        {
            int x=0;
            while(x+N<=g.w)
            {
                std::vector<int> pat(N*N);
                int dy=0;
                while(dy<N) { int dx=0; while(dx<N) { pat[dy*N+dx]=g.c[(x+dx)+(y+dy)*g.w]; dx++; } dy++; }
                std::map<std::vector<int>,int>::iterator it=patIndex.find(pat);
                if(it==patIndex.end()) { const int id=(int)pats.size(); patIndex[pat]=id; pats.push_back(pat); weight.push_back(1); }
                else weight[it->second]++;
                x++;
            }
            y++;
        }
        gi++;
    }
    const int P=(int)pats.size();
    Pout=P;
    if(P<1) { std::cerr << "WFCo: model maps smaller than " << N << "x" << N << std::endl; return false; }
    // memory guard: compat is W*H*P*4 ints
    const double cells=(double)W*H;
    if(cells*P*4.0 > 6.0e8) { std::cerr << "WFCo: " << P << " patterns x " << W << "x" << H
        << " too big; use smaller N or W/H" << std::endl; return false; }

    // 2) index patterns by edge signatures (cols/rows as a byte string) so
    //    compatibility is built without an O(P^2) scan.
    std::map<std::string,std::vector<int> > byLeft,byRight,byTop,byBottom;
    int pp=0;
    while(pp<P)
    {
        const std::vector<int> &pat=pats[pp];
        std::string L,R,T,B;
        int r=0;
        while(r<N)
        {
            int c=0;
            while(c<N)
            {
                const char ch=(char)pat[r*N+c];
                if(c<N-1) L.push_back(ch);   // cols 0..N-2
                if(c>0)   R.push_back(ch);    // cols 1..N-1
                if(r<N-1) T.push_back(ch);    // rows 0..N-2
                if(r>0)   B.push_back(ch);    // rows 1..N-1
                c++;
            }
            r++;
        }
        byLeft[L].push_back(pp); byRight[R].push_back(pp); byTop[T].push_back(pp); byBottom[B].push_back(pp);
        pp++;
    }
    // propagator[d][p] = patterns allowed as the d-neighbour of p (0=R 1=L 2=D 3=U)
    std::vector<std::vector<int> > prop[4];
    int d0=0; while(d0<4) { prop[d0].assign(P,std::vector<int>()); d0++; }
    pp=0;
    while(pp<P)
    {
        const std::vector<int> &pat=pats[pp];
        std::string L,R,T,B;
        int r=0;
        while(r<N)
        {
            int c=0;
            while(c<N)
            {
                const char ch=(char)pat[r*N+c];
                if(c<N-1) L.push_back(ch);
                if(c>0)   R.push_back(ch);
                if(r<N-1) T.push_back(ch);
                if(r>0)   B.push_back(ch);
                c++;
            }
            r++;
        }
        std::map<std::string,std::vector<int> >::iterator it;
        it=byLeft.find(R);   if(it!=byLeft.end())   prop[0][pp]=it->second;   // RIGHT nb: its LEFT cols == my RIGHT cols
        it=byRight.find(L);  if(it!=byRight.end())  prop[1][pp]=it->second;   // LEFT  nb: its RIGHT cols == my LEFT cols
        it=byTop.find(B);    if(it!=byTop.end())    prop[2][pp]=it->second;   // DOWN  nb: its TOP rows == my BOTTOM rows
        it=byBottom.find(T); if(it!=byBottom.end()) prop[3][pp]=it->second;   // UP    nb: its BOTTOM rows == my TOP rows
        pp++;
    }

    // base support counts depend only on (p,d): compat init = |prop[opp[d]][p]|
    const int opp[4]={1,0,3,2};
    std::vector<int> base(P*4);
    pp=0; while(pp<P) { int d=0; while(d<4) { base[pp*4+d]=(int)prop[opp[d]][pp].size(); d++; } pp++; }
    long totW=0; pp=0; while(pp<P) { totW+=weight[pp]; pp++; }

    const int NC=W*H;
    std::vector<char> possible((size_t)NC*P,1);
    std::vector<int> nPoss(NC,P);
    std::vector<long> sumW(NC,totW);
    std::vector<int> compat((size_t)NC*P*4,0);
    std::vector<std::pair<int,int> > banStack;
    result.assign(W*H,0);

    bool solved=false;
    int attempt=0;
    while(attempt<6 && !solved)
    {
        std::mt19937 rng(seed+(unsigned int)attempt);
        std::fill(possible.begin(),possible.end(),(char)1);
        { int c=0; while(c<NC) { nPoss[c]=P; sumW[c]=totW; c++; } }
        { int c=0; while(c<NC) { int q=0; while(q<P) { int d=0; while(d<4) { compat[((size_t)c*P+q)*4+d]=base[q*4+d]; d++; } q++; } c++; } }
        banStack.clear();
        bool contradiction=false;
        while(!contradiction)
        {
            // lowest-count uncollapsed cell
            int best=-1,bestN=P+1;
            int c=0;
            while(c<NC) { if(nPoss[c]>1 && nPoss[c]<bestN) { bestN=nPoss[c]; best=c; } c++; }
            if(best<0)
                break;   // every cell collapsed
            // weighted random pick among the cell's remaining patterns
            long pick=(long)(rng()%(unsigned long)(sumW[best]>0?sumW[best]:1));
            int chosen=-1;
            pp=0;
            while(pp<P) { if(possible[(size_t)best*P+pp]) { pick-=weight[pp]; if(pick<0) { chosen=pp; break; } } pp++; }
            if(chosen<0) { contradiction=true; break; }
            pp=0;
            while(pp<P) { if(pp!=chosen && possible[(size_t)best*P+pp]) wfcBanPattern(best,pp,P,possible,nPoss,sumW,weight,compat,banStack,contradiction); pp++; }
            wfcPropagate(P,W,H,prop,possible,nPoss,sumW,weight,compat,banStack,contradiction);
        }
        if(!contradiction)
        {
            bool ok=true;
            int c=0;
            while(c<NC) { int sel=-1,q=0; while(q<P) { if(possible[(size_t)c*P+q]) { sel=q; break; } q++; } if(sel<0) { ok=false; break; } result[c]=pats[sel][0]; c++; }
            if(ok)
                solved=true;
        }
        attempt++;
    }
    const char *mode = solved ? "AC4" : "greedy";
    if(!solved)
    {
        // GREEDY scanline fallback (always succeeds when strict WFC contradicts):
        // each cell picks a pattern compatible with its already-placed LEFT
        // (allowed RIGHT-neighbours) and UP (allowed DOWN-neighbours), weighted by
        // frequency; relax to one-sided / any if the intersection is empty, so it
        // never dead-ends.  Coherence stays high because each step honours overlap.
        std::mt19937 rng(seed+777u);
        std::vector<int> chosenPat(NC,-1);
        int c=0;
        while(c<NC)
        {
            const int x=c%W, y=c/W;
            const std::vector<int> *fromLeft=nullptr;
            const std::vector<int> *fromUp=nullptr;
            if(x>0 && chosenPat[c-1]>=0) fromLeft=&prop[0][chosenPat[c-1]];
            if(y>0 && chosenPat[c-W]>=0) fromUp=&prop[2][chosenPat[c-W]];
            std::vector<int> cand;
            if(fromLeft!=nullptr && fromUp!=nullptr)
            {
                std::set<int> s(fromUp->begin(),fromUp->end());
                size_t i=0;
                while(i<fromLeft->size()) { if(s.find((*fromLeft)[i])!=s.cend()) cand.push_back((*fromLeft)[i]); i++; }
                if(cand.empty())
                    cand = (rng()&1u) ? *fromLeft : *fromUp;   // relax to ONE edge, side chosen at random (no horizontal streak bias)
            }
            else if(fromLeft!=nullptr) cand=*fromLeft;
            else if(fromUp!=nullptr)   cand=*fromUp;
            if(cand.empty()) { int q=0; while(q<P) { cand.push_back(q); q++; } }   // first cell / fully relaxed
            long tot=0; size_t i=0;
            while(i<cand.size()) { tot+=weight[cand[i]]; i++; }
            long pick=(long)(rng()%(unsigned long)(tot>0?tot:1));
            int sel=cand[0];
            i=0;
            while(i<cand.size()) { pick-=weight[cand[i]]; if(pick<0) { sel=cand[i]; break; } i++; }
            chosenPat[c]=sel;
            result[c]=pats[sel][0];
            c++;
        }
    }

    modeOut=mode;
    return true;
}
static int runWfcOverlap(const QStringList &args)
{
    if(args.size()<2) { std::cerr << "wfco needs: <model-map-dir> <out.png> [N W H seed]" << std::endl; return 1; }
    int N = args.size()>2 ? args.at(2).toInt() : 2;   // 2x2 = best coherence/cost on the model data
    if(N<2) N=2;
    if(N>4) N=4;
    const int W = args.size()>3 ? args.at(3).toInt() : 30;
    const int H = args.size()>4 ? args.at(4).toInt() : 30;
    const unsigned int seed = args.size()>5 ? args.at(5).toUInt() : 1234u;

    std::map<std::string,int> catId;
    std::vector<std::string> cats;
    std::vector<WfcGrid> gs;
    std::vector<int> result;
    std::string mode;
    int P=0;
    if(!wfcGenerateGrid(args.at(0),N,W,H,seed,catId,cats,gs,result,mode,P))
        return 1;

    std::set<std::vector<int> > model2x2;
    wfcBuild2x2(gs,model2x2);
    const double coh=wfcCoherence(result,W,H,model2x2);

    QImage img(W*16,H*16,QImage::Format_ARGB32);
    img.fill(QColor(28,28,38));
    QPainter p(&img);
    int yy=0;
    while(yy<H)
    {
        int xx=0;
        while(xx<W)
        {
            const std::string &cat=cats.at(result[xx+yy*W]);
            if(!cat.empty())
                p.fillRect(xx*16,yy*16,16,16,MapDecoder::categoryColor(cat));
            xx++;
        }
        yy++;
    }
    p.end();
    if(!img.save(args.at(1))) { std::cerr << "cannot write " << args.at(1).toStdString() << std::endl; return 1; }
    char cohbuf[32]; std::snprintf(cohbuf,sizeof(cohbuf),"%.1f",coh);
    std::cout << "WFCo(" << N << "x" << N << " overlap, " << mode << ") " << W << "x" << H << " from " << gs.size()
              << " model map(s), " << P << " patterns, " << ((int)cats.size()-1) << " categories"
              << ", coherence " << cohbuf << "% -> " << args.at(1).toStdString() << std::endl;
    return 0;
}
// Generate COUNT coherent maps (overlapping WFC grid -> real .tmx on the target
// tileset) into an output dir.  The end-to-end transfer: learn structure from the
// model maps, place the target's REAL tagged tiles, write engine .tmx files.
static int runGenWfc(const QStringList &args)
{
    if(args.size()<3) { std::cerr << "genwfc needs: <model-map-dir> <target-tileset-dir> <out-dir> [count W H seed]" << std::endl; return 1; }
    const int count = args.size()>3 ? args.at(3).toInt() : 4;
    const int W = args.size()>4 ? args.at(4).toInt() : 40;
    const int H = args.size()>5 ? args.at(5).toInt() : 40;
    const unsigned int seed0 = args.size()>6 ? args.at(6).toUInt() : 1234u;
    QDir().mkpath(args.at(2));
    std::map<std::string,int> catId;
    std::vector<std::string> cats;
    std::vector<WfcGrid> gs;                 // decoded once, reused for every map
    int made=0,rc=0,i=0;
    while(i<count)
    {
        std::vector<int> idgrid;
        std::string mode;
        int P=0;
        if(!wfcGenerateGrid(args.at(0),2,W,H,seed0+(unsigned int)i*101u,catId,cats,gs,idgrid,mode,P))
            return 1;
        std::vector<std::string> grid(idgrid.size());
        size_t c=0;
        while(c<idgrid.size()) { grid[c]=cats.at(idgrid[c]); c++; }
        const QString outPath=QDir(args.at(2)).absoluteFilePath(QString("wfc-%1.tmx").arg(i,2,10,QChar('0')));
        if(writeTmxFromGrid(grid,W,H,args.at(1),outPath,seed0+(unsigned int)i)!=0) rc=1; else made++;
        i++;
    }
    std::cout << "genwfc: " << made << " coherent map(s) -> " << args.at(2).toStdString() << std::endl;
    return rc;
}
// Generate CITY maps only — the structures the terrain generator can't invent.
// Terrain (grass/water/routes) stays the job of map-procedural-generation-terrain;
// here we learn the building FOOTPRINTS from the model and lay a rigid town: ground
// + roads + building rectangles (roof on top, wall body, a door opening onto the
// road).  Real target tiles via the shared writeTmxFromGrid.  (Bots + interiors:
// next increment — they need the target's interior tilesets tagged.)
static int runGenCity(const QStringList &args)
{
    if(args.size()<3) { std::cerr << "gencity needs: <model-map-dir> <target-tileset-dir> <out-dir> [count W H seed]" << std::endl; return 1; }
    const int count = args.size()>3 ? args.at(3).toInt() : 4;
    const int W = args.size()>4 ? args.at(4).toInt() : 40;
    const int H = args.size()>5 ? args.at(5).toInt() : 40;
    const unsigned int seed0 = args.size()>6 ? args.at(6).toUInt() : 1234u;

    // 1) LEARN building footprint sizes from the model (bounding boxes of building
    //    blobs >=4 cells, clamped to a plausible house range).
    std::set<std::string> bcats;
    bcats.insert("building-wall"); bcats.insert("building-roof"); bcats.insert("door"); bcats.insert("window");
    std::vector<std::pair<int,int> > sizes;
    {
        MapDecoder dec;
        int scanned=0;
        QDirIterator it(args.at(0),QStringList()<<"*.tmx",QDir::Files,QDirIterator::Subdirectories);
        while(it.hasNext() && scanned<500)
        {
            MapDecoder::Result r;
            QString err;
            if(dec.decode(it.next(),r,err))
            {
                scanned++;
                const std::vector<Comp> bc=components(r.categoryGrid,r.w,r.h,bcats);
                size_t k=0;
                while(k<bc.size())
                {
                    const Comp &c=bc[k];
                    if(c.count>=4)
                    {
                        const int bw=c.maxx-c.minx+1, bh=c.maxy-c.miny+1;
                        if(bw>=3 && bw<=8 && bh>=3 && bh<=8) sizes.push_back(std::pair<int,int>(bw,bh));
                    }
                    k++;
                }
            }
        }
    }
    if(sizes.empty())
    {
        const int dw[3]={4,5,6}, dh[3]={4,4,5};
        int k=0; while(k<3) { sizes.push_back(std::pair<int,int>(dw[k],dh[k])); k++; }
    }
    std::cout << "gencity: learned " << sizes.size() << " building footprint(s) from the model" << std::endl;

    QDir().mkpath(args.at(2));
    int made=0,i=0;
    while(i<count)
    {
        std::mt19937 rng(seed0+(unsigned int)i*131u);
        std::vector<std::string> grid(W*H,std::string("ground"));
        std::vector<std::pair<int,int> > botCand;   // road cell in front of each door
        const int margin=3, PLOTW=8, PLOTH=7;
        int py=margin;
        while(py+PLOTH<=H-margin)
        {
            int x=margin;
            while(x+PLOTW<=W-margin)
            {
                const std::pair<int,int> &sz=sizes[rng()%sizes.size()];
                int bw=sz.first, bh=sz.second;
                if(bw>PLOTW-1) bw=PLOTW-1;
                if(bh>PLOTH-1) bh=PLOTH-1;
                const int bx=x+(PLOTW-bw)/2, by=py;
                int yy=0;
                while(yy<bh)
                {
                    int xx=0;
                    while(xx<bw)
                    {
                        const int gx=bx+xx, gy=by+yy;
                        if(gx<W && gy<H)
                            grid[gx+gy*W] = (yy==0) ? std::string("building-roof") : std::string("building-wall");
                        xx++;
                    }
                    yy++;
                }
                // door opening at the bottom-centre (path onto the road)
                const int dx=bx+bw/2, dy=by+bh-1;
                if(dx<W && dy<H) grid[dx+dy*W]=std::string("path");
                const int broady=py+PLOTH-1;       // the road row in front of this door
                if(dx<W && broady<H) botCand.push_back(std::pair<int,int>(dx,broady));
                x+=PLOTW;
            }
            // horizontal road below this building row
            const int ry=py+PLOTH-1;
            int rx=margin;
            while(rx<W-margin) { if(ry<H) grid[rx+ry*W]=std::string("path"); rx++; }
            py+=PLOTH;
        }
        // vertical main road, only over ground (never carve through a building)
        const int vc=W/2;
        int vy=margin;
        while(vy<H-margin) { if(grid[vc+vy*W]=="ground") grid[vc+vy*W]=std::string("path"); vy++; }

        // place a bot in front of ~1 in 3 buildings (an NPC standing on the street)
        std::vector<std::pair<int,int> > bots;
        {
            size_t k=0;
            while(k<botCand.size()) { if(rng()%3==0 && bots.size()<12) bots.push_back(botCand[k]); k++; }
        }
        const QString outPath=QDir(args.at(2)).absoluteFilePath(QString("city-%1.tmx").arg(i,2,10,QChar('0')));
        if(writeTmxFromGrid(grid,W,H,args.at(1),outPath,seed0+(unsigned int)i,bots)==0)
        {
            // sibling <map>.xml with one generic NPC dialog per bot id
            const QString xmlPath=QDir(args.at(2)).absoluteFilePath(QString("city-%1.xml").arg(i,2,10,QChar('0')));
            QFile xf(xmlPath);
            if(xf.open(QIODevice::WriteOnly|QIODevice::Truncate))
            {
                QTextStream xw(&xf);
                xw << "<map zone=\"generated\" type=\"city\">\n";
                xw << " <name>Generated City " << i << "</name>\n";
                size_t b=0;
                while(b<bots.size())
                {
                    xw << " <bot id=\"" << (b+1) << "\">\n";
                    xw << "  <step type=\"text\"><text><![CDATA[Welcome to our town!]]></text></step>\n";
                    xw << " </bot>\n";
                    b++;
                }
                xw << "</map>\n";
                xf.close();
            }
            made++;
        }
        i++;
    }
    std::cout << "gencity: " << made << " city map(s) -> " << args.at(2).toStdString() << std::endl;
    return 0;
}

int main(int argc, char *argv[])
{
    QStringList args;
    int i=1;
    while(i<argc) { args.append(QString::fromUtf8(argv[i])); i++; }
    const bool cli = !args.isEmpty()
        && (args.at(0)=="--guard" || args.at(0)=="--tag" || args.at(0)=="--selftest"
            || args.at(0)=="--usage" || args.at(0)=="--suggest" || args.at(0)=="--classify"
            || args.at(0)=="--decode" || args.at(0)=="--learn" || args.at(0)=="--verify" || args.at(0)=="--structure" || args.at(0)=="--generate" || args.at(0)=="--genmap"
            || args.at(0)=="--evalsuggest" || args.at(0)=="--evalknn" || args.at(0)=="--maptensor" || args.at(0)=="--wfc" || args.at(0)=="--wfco" || args.at(0)=="--genwfc" || args.at(0)=="--gencity" || args.at(0)=="--catlayers" || args.at(0)=="--usedtiles");
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
    if(!args.isEmpty() && args.at(0)=="--evalsuggest")
        return runEvalSuggest(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--evalknn")
        return runEvalKnn(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--maptensor")
        return runMapTensor(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--wfc")
        return runWfc(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--wfco")
        return runWfcOverlap(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--catlayers")
        return runCatLayers(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--genwfc")
        return runGenWfc(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--gencity")
        return runGenCity(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--usedtiles")
        return runUsedTiles(args.mid(1));
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
    if(!args.isEmpty() && args.at(0)=="--genmap")
        return runGenMap(args.mid(1));

    // GUI: optional first arg is a .tsx OR a directory of tilesets.
    MainWindow window;
    window.show();
    if(!args.isEmpty() && !args.at(0).startsWith("--"))
    {
        window.hideOpenButton();   // folder/file came from the CLI -> no Open button
        window.openPath(args.at(0));
    }
    return app.exec();
}
