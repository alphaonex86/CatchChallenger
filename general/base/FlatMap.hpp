#ifndef CATCHCHALLENGER_FLATMAP_H
#define CATCHCHALLENGER_FLATMAP_H

// The datapack's key -> value structures in the CATCHCHALLENGER_LOWRAM profile
// (see GeneralType.hpp): one sorted contiguous array, binary search. Measured by
// benchmark/benchmarkcontainers on the ESP32-C6 and the i486: ~1/3 of the heap
// of std::unordered_map (one block instead of one per entry), lookups 1.5-2x
// slower. The API is the subset of std::unordered_map the code uses, with the
// same value_type (std::pair<const K,V>), so the same code compiles on both.
// Serialized like std::unordered_map (count + key/value pairs): every build
// reads every other build's cache, and the ESP32 reads what the host emits.

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>
#include "../hps/serializer.h"
#include "../hps/basic_type/uint_serializer.h"

namespace CatchChallenger {

template <class K,class V>
class FlatMap
{
public:
    typedef K key_type;
    typedef V mapped_type;
    typedef std::pair<const K,V> value_type;
    typedef typename std::vector<value_type>::iterator iterator;
    typedef typename std::vector<value_type>::const_iterator const_iterator;
    typedef size_t size_type;

    FlatMap() {}
    FlatMap(const FlatMap &other) : items(other.items) {}
    FlatMap(FlatMap &&other) noexcept : items(std::move(other.items)) {}
    // a std::pair<const K,V> cannot be assigned: copy-construct then swap
    FlatMap &operator=(const FlatMap &other)
    {
        if(this!=&other)
        {
            std::vector<value_type> copy(other.items);
            items.swap(copy);
        }
        return *this;
    }
    FlatMap &operator=(FlatMap &&other) noexcept
    {
        items.swap(other.items);
        other.items.clear();
        return *this;
    }

    iterator begin() { return items.begin(); }
    iterator end() { return items.end(); }
    const_iterator begin() const { return items.cbegin(); }
    const_iterator end() const { return items.cend(); }
    const_iterator cbegin() const { return items.cbegin(); }
    const_iterator cend() const { return items.cend(); }
    size_t size() const { return items.size(); }
    bool empty() const { return items.empty(); }
    void clear() { items.clear(); }
    void reserve(const size_t &count) { items.reserve(count); }
    bool operator==(const FlatMap &other) const { return items==other.items; }
    bool operator!=(const FlatMap &other) const { return items!=other.items; }

    iterator find(const K &key)
    {
        const size_t index=lowerBound(key);
        if(index<items.size() && items[index].first==key)
            return items.begin()+static_cast<std::ptrdiff_t>(index);
        return items.end();
    }
    const_iterator find(const K &key) const
    {
        const size_t index=lowerBound(key);
        if(index<items.size() && items[index].first==key)
            return items.cbegin()+static_cast<std::ptrdiff_t>(index);
        return items.cend();
    }
    size_t count(const K &key) const { return find(key)==items.cend()?0:1; }
    bool contains(const K &key) const { return find(key)!=items.cend(); }

    V &at(const K &key)
    {
        iterator it=find(key);
        if(it==items.end())
            missingKey();
        return it->second;
    }
    const V &at(const K &key) const
    {
        const_iterator it=find(key);
        if(it==items.cend())
            missingKey();
        return it->second;
    }
    V &operator[](const K &key)
    {
        const size_t index=lowerBound(key);
        if(index<items.size() && items[index].first==key)
            return items[index].second;
        return insertAt(index,value_type(key,V()))->second;
    }
    std::pair<iterator,bool> insert(const value_type &value)
    {
        const size_t index=lowerBound(value.first);
        if(index<items.size() && items[index].first==value.first)
            return std::pair<iterator,bool>(items.begin()+static_cast<std::ptrdiff_t>(index),false);
        return std::pair<iterator,bool>(insertAt(index,value_type(value)),true);
    }
    std::pair<iterator,bool> insert(const std::pair<K,V> &value)
    {
        return insert(value_type(value.first,value.second));
    }
    std::pair<iterator,bool> emplace(const K &key,const V &value)
    {
        return insert(value_type(key,value));
    }
    size_t erase(const K &key)
    {
        const_iterator it=find(key);
        if(it==items.cend())
            return 0;
        erase(it);
        return 1;
    }
    iterator erase(const_iterator position)
    {
        const size_t index=static_cast<size_t>(position-items.cbegin());
        rebuild(index,nullptr,true);
        return items.begin()+static_cast<std::ptrdiff_t>(index);
    }

    // hps: bulk load from (key, value) pairs in any order, the last duplicate wins
    void assignUnsorted(std::vector<std::pair<K,V> > &pairs)
    {
        std::stable_sort(pairs.begin(),pairs.end(),KeyLess());
        std::vector<value_type> sorted;
        sorted.reserve(pairs.size());
        size_t index=0;
        while(index<pairs.size())
        {
            if(index+1>=pairs.size() || pairs[index+1].first!=pairs[index].first)
                sorted.emplace_back(std::move(pairs[index].first),std::move(pairs[index].second));
            index++;
        }
        items.swap(sorted);
    }
private:
    // pair<const K,V> cannot be assigned, so a vector of it cannot shift in place:
    // appending in key order (the loaders' case) is O(1), anything else rebuilds.
    std::vector<value_type> items;

    struct KeyLess
    {
        bool operator()(const std::pair<K,V> &a,const std::pair<K,V> &b) const { return a.first<b.first; }
    };

    size_t lowerBound(const K &key) const
    {
        size_t low=0;
        size_t high=items.size();
        while(low<high)
        {
            const size_t middle=low+(high-low)/2;
            if(items[middle].first<key)
                low=middle+1;
            else
                high=middle;
        }
        return low;
    }
    iterator insertAt(const size_t &index,value_type value)
    {
        if(index==items.size())
        {
            items.emplace_back(std::move(value));
            return items.end()-1;
        }
        rebuild(index,&value,false);
        return items.begin()+static_cast<std::ptrdiff_t>(index);
    }
    // a copy of `items` with `value` inserted at, or the element removed from, `index`
    void rebuild(const size_t &index,value_type *value,const bool &remove)
    {
        std::vector<value_type> rebuilt;
        rebuilt.reserve(remove?items.size()-1:items.size()+1);
        size_t position=0;
        while(position<items.size())
        {
            if(position==index && !remove)
                rebuilt.emplace_back(std::move(*value));
            if(position!=index || !remove)
                rebuilt.emplace_back(std::move(items[position]));
            position++;
        }
        items.swap(rebuilt);
    }
    static void missingKey()
    {
        std::cerr << "FlatMap::at(): key not found (abort)" << std::endl;
        abort();
    }
};

// The datapack's sets in the CATCHCHALLENGER_LOWRAM profile: one sorted array.
template <class K>
class FlatSet
{
public:
    typedef K key_type;
    typedef K value_type;
    typedef typename std::vector<K>::const_iterator iterator;
    typedef typename std::vector<K>::const_iterator const_iterator;
    typedef size_t size_type;

    FlatSet() {}
    template <class Iterator>
    FlatSet(Iterator first,Iterator last) : items(first,last)
    {
        std::sort(items.begin(),items.end());
        items.erase(std::unique(items.begin(),items.end()),items.end());
    }

    const_iterator begin() const { return items.cbegin(); }
    const_iterator end() const { return items.cend(); }
    const_iterator cbegin() const { return items.cbegin(); }
    const_iterator cend() const { return items.cend(); }
    size_t size() const { return items.size(); }
    bool empty() const { return items.empty(); }
    void clear() { items.clear(); }
    void reserve(const size_t &count) { items.reserve(count); }
    bool operator==(const FlatSet &other) const { return items==other.items; }
    bool operator!=(const FlatSet &other) const { return items!=other.items; }

    const_iterator find(const K &key) const
    {
        const_iterator it=std::lower_bound(items.cbegin(),items.cend(),key);
        if(it!=items.cend() && *it==key)
            return it;
        return items.cend();
    }
    size_t count(const K &key) const { return find(key)==items.cend()?0:1; }
    bool contains(const K &key) const { return find(key)!=items.cend(); }
    std::pair<const_iterator,bool> insert(const K &key)
    {
        typename std::vector<K>::iterator it=std::lower_bound(items.begin(),items.end(),key);
        if(it!=items.end() && *it==key)
            return std::pair<const_iterator,bool>(it,false);
        it=items.insert(it,key);
        return std::pair<const_iterator,bool>(it,true);
    }
    size_t erase(const K &key)
    {
        typename std::vector<K>::iterator it=std::lower_bound(items.begin(),items.end(),key);
        if(it==items.end() || !(*it==key))
            return 0;
        items.erase(it);
        return 1;
    }
    const_iterator erase(const_iterator position)
    {
        return items.erase(position);
    }
    // hps: bulk load in any order, duplicates dropped
    void assignUnsorted(std::vector<K> &keys)
    {
        std::sort(keys.begin(),keys.end());
        keys.erase(std::unique(keys.begin(),keys.end()),keys.end());
        items.swap(keys);
    }
private:
    std::vector<K> items;
};

}

namespace hps {
// Same bytes as hps' std::unordered_map / std::unordered_set serializers.
template <class K,class V,class B>
class Serializer<CatchChallenger::FlatMap<K,V>,B>
{
public:
    static void serialize(const CatchChallenger::FlatMap<K,V> &container,B &ob)
    {
        Serializer<size_t,B>::serialize(container.size(),ob);
        typename CatchChallenger::FlatMap<K,V>::const_iterator it=container.cbegin();
        while(it!=container.cend())
        {
            Serializer<K,B>::serialize(it->first,ob);
            Serializer<V,B>::serialize(it->second,ob);
            ++it;
        }
    }
    static void parse(CatchChallenger::FlatMap<K,V> &container,B &ib)
    {
        size_t count=0;
        Serializer<size_t,B>::parse(count,ib);
        std::vector<std::pair<K,V> > pairs(count);
        size_t index=0;
        while(index<count)
        {
            Serializer<K,B>::parse(pairs[index].first,ib);
            Serializer<V,B>::parse(pairs[index].second,ib);
            index++;
        }
        container.assignUnsorted(pairs);
    }
};
template <class K,class B>
class Serializer<CatchChallenger::FlatSet<K>,B>
{
public:
    static void serialize(const CatchChallenger::FlatSet<K> &container,B &ob)
    {
        Serializer<size_t,B>::serialize(container.size(),ob);
        typename CatchChallenger::FlatSet<K>::const_iterator it=container.cbegin();
        while(it!=container.cend())
        {
            Serializer<K,B>::serialize(*it,ob);
            ++it;
        }
    }
    static void parse(CatchChallenger::FlatSet<K> &container,B &ib)
    {
        size_t count=0;
        Serializer<size_t,B>::parse(count,ib);
        std::vector<K> keys(count);
        size_t index=0;
        while(index<count)
        {
            Serializer<K,B>::parse(keys[index],ib);
            index++;
        }
        container.assignUnsorted(keys);
    }
};
}

#endif
