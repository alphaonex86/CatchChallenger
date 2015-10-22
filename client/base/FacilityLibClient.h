#ifndef CATCHCHALLENGER_FACILITYLIBClient_H
#define CATCHCHALLENGER_FACILITYLIBClient_H

#include <QString>
#include <QHash>
#include <unordered_map>

namespace CatchChallenger {
class FacilityLibClient
{
public:
    static QString timeToString(const uint32_t &playerMonster);
};

template <class T, class U>
QHash<T,U> stdmapToQHash(const std::unordered_map<T,U> &map)
{
    QHash<T,U> hash;
    auto i=map.begin();
    while(i!=map.cend())
    {
        hash[i->first]=i->second;
        ++i;
    }
    return hash;
}

template <class T>
QSet<T> stdunorderedsetToQSet(const std::unordered_set<T> &oldset)
{
    QSet<T> set;
    auto i=oldset.begin();
    while(i!=oldset.cend())
    {
        set << *i;
        ++i;
    }
    return set;
}

template <class T>
QList<T> stdvectorToQList(const std::vector<T> &vector)
{
    QList<T> list;
    unsigned int index=0;
    while(index<vector.size())
    {
        list << vector.at(index);
        ++index;
    }
    return list;
}

}

#endif
