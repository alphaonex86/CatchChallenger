#include "Naming.hpp"
#include "Gen3Script.hpp"
#include "Gen3Text.hpp"

#include <algorithm>
#include <iostream>
#include <queue>
#include <set>

Naming::Naming(const GbaRom &rom, const Decoder &decoder) :
    rom_(rom),
    decoder_(decoder),
    empty_()
{
}

uint16_t Naming::keyOf(uint8_t group, uint8_t map)
{
    return static_cast<uint16_t>((group<<8)|map);
}

static uint32_t ufFind(std::unordered_map<uint32_t,uint32_t> &uf, uint32_t x)
{
    while(uf[x]!=x)
    {
        uf[x]=uf[uf[x]];
        x=uf[x];
    }
    return x;
}

std::string Naming::sectionName(uint8_t sid) const
{
    const GameInfo &gi=rom_.game();
    if(gi.mapNameTable==0 || sid<gi.mapNameMinSid || sid>gi.mapNameMaxSid)
        return std::string();
    uint32_t entry=gi.mapNameTable+static_cast<uint32_t>(sid)*gi.mapNameStride+gi.mapNameField;
    bool ok=false;
    uint32_t nameOff=rom_.pointer(entry,&ok);
    if(!ok)
        return std::string();
    return Gen3Text::decode(rom_,nameOff,24);
}

bool Naming::isArea(const DecodedMap &m) const
{
    // town(1) city(2) route(3) underground(4) underwater(5) ocean(6)
    return m.mapType>=1 && m.mapType<=6;
}

bool Naming::looksLikeHouse(const DecodedMap &m) const
{
    if(m.width>12 || m.height>10)
        return false;
    // an exit warp (teleport-on-push/it, not a door) on the bottom two rows
    bool bottomExit=false;
    size_t wi=0;
    while(wi<m.warps.size())
    {
        const DecodedWarp &w=m.warps[wi];
        if(static_cast<int>(w.y)>=m.height-2)
        {
            std::string wc=decoder_.warpClassAt(m,w);
            if(wc=="teleport on push" || wc=="teleport on it")
            {
                bottomExit=true;
                break;
            }
        }
        wi++;
    }
    if(!bottomExit)
        return false;
    // not a shop and no trainer fight (which would make it a gym / special place)
    Gen3Script script(rom_);
    size_t ni=0;
    while(ni<m.npcs.size())
    {
        const DecodedNpc &n=m.npcs[ni];
        ScriptResult r=script.classify(n.trainerType,n.scriptPtr);
        if(r.kind==BotKind::Fight || r.kind==BotKind::Mart)
            return false;
        ni++;
    }
    return true;
}

int Naming::namedSidOf(uint16_t key) const
{
    const DecodedMap *m=decoder_.find(static_cast<uint8_t>(key>>8),static_cast<uint8_t>(key & 0xFF));
    if(m==nullptr)
        return -1;
    if(isArea(*m) && !sectionName(m->regionSection).empty())
        return m->regionSection;
    // Breadth-first over warps to the nearest NAMED area map.
    std::set<uint16_t> seen;
    std::queue<uint16_t> q;
    q.push(key);
    seen.insert(key);
    while(!q.empty())
    {
        uint16_t k=q.front();
        q.pop();
        std::unordered_map<uint16_t,std::vector<uint16_t> >::const_iterator it=adj_.find(k);
        if(it!=adj_.cend())
        {
            size_t i=0;
            while(i<it->second.size())
            {
                uint16_t n=it->second[i];
                if(seen.find(n)==seen.cend())
                {
                    const DecodedMap *nm=decoder_.find(static_cast<uint8_t>(n>>8),static_cast<uint8_t>(n & 0xFF));
                    if(nm!=nullptr && isArea(*nm) && !sectionName(nm->regionSection).empty())
                        return nm->regionSection;
                    seen.insert(n);
                    q.push(n);
                }
                i++;
            }
        }
    }
    return -1; // orphan
}

void Naming::build()
{
    const std::vector<DecodedMap> &maps=decoder_.maps();

    // Warp adjacency (undirected).
    size_t i=0;
    while(i<maps.size())
    {
        const DecodedMap &m=maps[i];
        uint16_t k=keyOf(m.group,m.map);
        size_t w=0;
        while(w<m.warps.size())
        {
            const DecodedWarp &wp=m.warps[w];
            if(decoder_.find(wp.destGroup,wp.destMap)!=nullptr)
            {
                uint16_t dk=keyOf(wp.destGroup,wp.destMap);
                adj_[k].push_back(dk);
                adj_[dk].push_back(k);
            }
            w++;
        }
        // connections (borders + dive/emerge) also make maps adjacent
        size_t cc=0;
        while(cc<m.connections.size())
        {
            const DecodedConnection &cn=m.connections[cc];
            if(cn.direction>=1 && cn.direction<=6 && decoder_.find(cn.destGroup,cn.destMap)!=nullptr)
            {
                uint16_t dk=keyOf(cn.destGroup,cn.destMap);
                adj_[k].push_back(dk);
                adj_[dk].push_back(k);
            }
            cc++;
        }
        i++;
    }

    // Named-area section id for every map (its own, or the nearest reachable
    // named area).  Maps sharing a section id share one area folder.
    std::unordered_map<uint16_t,int> sidOf;
    i=0;
    while(i<maps.size())
    {
        uint16_t k=keyOf(maps[i].group,maps[i].map);
        sidOf[k]=namedSidOf(k);
        i++;
    }

    // Orphans (no named area reachable) are grouped by warp-connected component
    // so a disconnected multi-map area stays together.
    std::unordered_map<uint16_t,int> orphanComp;
    int nextComp=0;
    i=0;
    while(i<maps.size())
    {
        uint16_t k=keyOf(maps[i].group,maps[i].map);
        if(sidOf[k]<0 && orphanComp.find(k)==orphanComp.cend())
        {
            int comp=nextComp++;
            std::queue<uint16_t> q;
            q.push(k);
            orphanComp[k]=comp;
            while(!q.empty())
            {
                uint16_t c=q.front();
                q.pop();
                std::unordered_map<uint16_t,std::vector<uint16_t> >::const_iterator it=adj_.find(c);
                if(it!=adj_.cend())
                {
                    size_t j=0;
                    while(j<it->second.size())
                    {
                        uint16_t n=it->second[j];
                        if(sidOf.find(n)!=sidOf.cend() && sidOf[n]<0 && orphanComp.find(n)==orphanComp.cend())
                        {
                            orphanComp[n]=comp;
                            q.push(n);
                        }
                        j++;
                    }
                }
            }
        }
        i++;
    }

    // Group id per map: the named section id, or 0x10000+component for orphans.
    std::unordered_map<uint16_t,uint32_t> groupOf;
    std::set<uint32_t> groups;
    i=0;
    while(i<maps.size())
    {
        uint16_t k=keyOf(maps[i].group,maps[i].map);
        uint32_t g=(sidOf[k]>=0) ? static_cast<uint32_t>(sidOf[k])
                                 : (0x10000u+static_cast<uint32_t>(orphanComp[k]));
        groupOf[k]=g;
        groups.insert(g);
        i++;
    }

    // Merge area groups that are adjacent AND share the same section name (the
    // several "Underwater" sections that connect to each other become one).
    std::unordered_map<uint32_t,std::string> rawName;
    std::set<uint32_t>::const_iterator rg=groups.cbegin();
    while(rg!=groups.cend())
    {
        rawName[*rg]=(*rg<0x10000u) ? sectionName(static_cast<uint8_t>(*rg)) : std::string();
        ++rg;
    }
    std::unordered_map<uint32_t,uint32_t> uf;
    rg=groups.cbegin();
    while(rg!=groups.cend())
    {
        uf[*rg]=*rg;
        ++rg;
    }
    i=0;
    while(i<maps.size())
    {
        uint16_t k=keyOf(maps[i].group,maps[i].map);
        uint32_t ga=groupOf[k];
        std::unordered_map<uint16_t,std::vector<uint16_t> >::const_iterator it=adj_.find(k);
        if(it!=adj_.cend())
        {
            size_t j=0;
            while(j<it->second.size())
            {
                std::unordered_map<uint16_t,uint32_t>::const_iterator gn=groupOf.find(it->second[j]);
                if(gn!=groupOf.cend())
                {
                    uint32_t gb=gn->second;
                    if(ga!=gb && !rawName[ga].empty() && rawName[ga]==rawName[gb])
                    {
                        uint32_t ra=ufFind(uf,ga),rb=ufFind(uf,gb);
                        if(ra!=rb)
                            uf[ra]=rb;
                    }
                }
                j++;
            }
        }
        i++;
    }
    std::set<uint32_t> mergedGroups;
    i=0;
    while(i<maps.size())
    {
        uint16_t k=keyOf(maps[i].group,maps[i].map);
        uint32_t r=ufFind(uf,groupOf[k]);
        groupOf[k]=r;
        mergedGroups.insert(r);
        i++;
    }
    groups=mergedGroups;

    // Slug + display per area group (deterministic order, unique slugs).
    std::unordered_map<uint32_t,std::string> areaSlug,areaDisplay;
    std::set<std::string> usedSlugs;
    int unnamed=0;
    std::set<uint32_t>::const_iterator gk=groups.cbegin();
    while(gk!=groups.cend())
    {
        std::string name;
        if(*gk<0x10000u)
            name=sectionName(static_cast<uint8_t>(*gk));
        std::string slug=name.empty() ? (std::string("area-")+std::to_string(unnamed++)) : Gen3Text::slug(name);
        std::string base=slug;
        int dup=2;
        while(usedSlugs.find(slug)!=usedSlugs.cend())
        {
            slug=base+"-"+std::to_string(dup);
            dup++;
        }
        usedSlugs.insert(slug);
        areaSlug[*gk]=slug;
        areaDisplay[*gk]=name.empty() ? slug : Gen3Text::display(name);
        zones_.push_back(std::make_pair(slug,areaDisplay[*gk]));
        ++gk;
    }

    // Group every map by its area group, then assign file names within each.
    std::unordered_map<uint32_t,std::vector<uint16_t> > byArea;
    i=0;
    while(i<maps.size())
    {
        uint16_t k=keyOf(maps[i].group,maps[i].map);
        byArea[groupOf[k]].push_back(k);
        i++;
    }

    std::unordered_map<uint32_t,std::vector<uint16_t> >::iterator bit=byArea.begin();
    while(bit!=byArea.end())
    {
        uint32_t A=bit->first;
        std::vector<uint16_t> members=bit->second;
        std::sort(members.begin(),members.end());
        // Region folder per area: named areas (A is the section id) may fall in
        // the game's secondary region (e.g. the Sevii Islands); the rest use the
        // primary region.
        std::string region=(A<0x10000u) ? rom_.game().regionFor(static_cast<uint8_t>(A))
                                         : rom_.game().region;
        std::string folder=region+"/"+areaSlug[A];

        std::vector<uint16_t> areaMaps,indoorMaps;
        size_t mi=0;
        while(mi<members.size())
        {
            const DecodedMap *cm=decoder_.find(static_cast<uint8_t>(members[mi]>>8),static_cast<uint8_t>(members[mi] & 0xFF));
            if(cm!=nullptr && isArea(*cm))
                areaMaps.push_back(members[mi]);
            else
                indoorMaps.push_back(members[mi]);
            mi++;
        }

        // outdoor/area maps: <area>, <area>-2, ...
        size_t an=0;
        while(an<areaMaps.size())
        {
            std::string fn=areaSlug[A];
            std::string disp=areaDisplay[A];
            if(an>0)
            {
                fn+="-"+std::to_string(an+1);
                disp+=" "+std::to_string(an+1);
            }
            path_[areaMaps[an]]=folder+"/"+fn;
            zone_[areaMaps[an]]=areaSlug[A];
            display_[areaMaps[an]]=disp;
            an++;
        }

        // Indoor maps grouped into buildings (connected components via warps).
        // Each is named: a distinct section name; else a "house" for a small
        // generic interior (<=12x10, bottom exit, no shop/fight); else building-N.
        std::set<uint16_t> indoorSet(indoorMaps.begin(),indoorMaps.end());
        std::set<uint16_t> visited;
        std::vector<std::vector<uint16_t> > comps;
        std::vector<std::string> compName,compDisplay;
        std::vector<char> compHouse;
        size_t ii=0;
        while(ii<indoorMaps.size())
        {
            uint16_t start=indoorMaps[ii];
            ii++;
            if(visited.find(start)!=visited.cend())
                continue;
            std::vector<uint16_t> comp;
            std::queue<uint16_t> q;
            q.push(start);
            visited.insert(start);
            while(!q.empty())
            {
                uint16_t c=q.front();
                q.pop();
                comp.push_back(c);
                std::unordered_map<uint16_t,std::vector<uint16_t> >::const_iterator it=adj_.find(c);
                if(it!=adj_.cend())
                {
                    size_t j=0;
                    while(j<it->second.size())
                    {
                        uint16_t n=it->second[j];
                        if(indoorSet.find(n)!=indoorSet.cend() && visited.find(n)==visited.cend())
                        {
                            visited.insert(n);
                            q.push(n);
                        }
                        j++;
                    }
                }
            }
            std::sort(comp.begin(),comp.end());
            std::string sname,sdisp;
            size_t ci=0;
            while(ci<comp.size())
            {
                const DecodedMap *cm=decoder_.find(static_cast<uint8_t>(comp[ci]>>8),static_cast<uint8_t>(comp[ci] & 0xFF));
                std::string sn=(cm!=nullptr) ? sectionName(cm->regionSection) : std::string();
                if(!sn.empty() && Gen3Text::slug(sn)!=areaSlug[A])
                {
                    sname=Gen3Text::slug(sn);
                    sdisp=Gen3Text::display(sn);
                    break;
                }
                ci++;
            }
            bool house=false;
            if(sname.empty() && comp.size()==1)
            {
                const DecodedMap *cm=decoder_.find(static_cast<uint8_t>(comp[0]>>8),static_cast<uint8_t>(comp[0] & 0xFF));
                if(cm!=nullptr && looksLikeHouse(*cm))
                    house=true;
            }
            comps.push_back(comp);
            compName.push_back(sname);
            compDisplay.push_back(sdisp);
            compHouse.push_back(house ? 1 : 0);
        }
        int houseTotal=0;
        {
            size_t k=0;
            while(k<compHouse.size())
            {
                if(compHouse[k])
                    houseTotal++;
                k++;
            }
        }
        int buildingIndex=0,houseIndex=0;
        size_t cc=0;
        while(cc<comps.size())
        {
            const std::vector<uint16_t> &comp=comps[cc];
            std::string bname=compName[cc],bdisplay=compDisplay[cc];
            if(bname.empty())
            {
                if(compHouse[cc])
                {
                    houseIndex++;
                    bname=(houseTotal==1) ? std::string("house") : ("house-"+std::to_string(houseIndex));
                    bdisplay=(houseTotal==1) ? (areaDisplay[A]+" - House")
                                             : (areaDisplay[A]+" - House "+std::to_string(houseIndex));
                }
                else
                {
                    buildingIndex++;
                    bname="building-"+std::to_string(buildingIndex);
                    bdisplay=areaDisplay[A]+" - Building "+std::to_string(buildingIndex);
                }
            }
            if(comp.size()==1)
            {
                path_[comp[0]]=folder+"/"+bname;
                zone_[comp[0]]=areaSlug[A];
                display_[comp[0]]=bdisplay;
            }
            else
            {
                size_t f=0;
                while(f<comp.size())
                {
                    path_[comp[f]]=folder+"/"+bname+"/floor-"+std::to_string(f);
                    zone_[comp[f]]=areaSlug[A];
                    display_[comp[f]]=bdisplay+" floor "+std::to_string(f);
                    f++;
                }
            }
            cc++;
        }

        ++bit;
    }

    std::cout << "Naming: " << zones_.size() << " areas, " << path_.size() << " maps placed" << std::endl;
}

const std::string &Naming::pathFor(uint8_t group, uint8_t map) const
{
    std::unordered_map<uint16_t,std::string>::const_iterator it=path_.find(keyOf(group,map));
    return it==path_.cend() ? empty_ : it->second;
}

const std::string &Naming::zoneFor(uint8_t group, uint8_t map) const
{
    std::unordered_map<uint16_t,std::string>::const_iterator it=zone_.find(keyOf(group,map));
    return it==zone_.cend() ? empty_ : it->second;
}

const std::string &Naming::displayFor(uint8_t group, uint8_t map) const
{
    std::unordered_map<uint16_t,std::string>::const_iterator it=display_.find(keyOf(group,map));
    return it==display_.cend() ? empty_ : it->second;
}

std::string Naming::typeFor(uint8_t group, uint8_t map) const
{
    const DecodedMap *m=decoder_.find(group,map);
    if(m==nullptr)
        return std::string("indoor");
    if(m->mapType==1 || m->mapType==2)
        return std::string("city");
    if(m->mapType>=3 && m->mapType<=6)
        return std::string("outdoor");
    return std::string("indoor");
}

const std::vector<std::pair<std::string,std::string> > &Naming::zones() const
{
    return zones_;
}
