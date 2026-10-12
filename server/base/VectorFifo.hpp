#ifndef CATCHCHALLENGER_VECTORFIFO_H
#define CATCHCHALLENGER_VECTORFIFO_H

#include <vector>
#include <cstddef>

namespace CatchChallenger {
// The std::queue calls the Client uses, on a std::vector: no heap while empty
// and no allocation per push once grown (std::deque holds ~550 B even empty,
// std::list allocates on every push). For the few-entry per-player queues of
// the CATCHCHALLENGER_LOWRAM profile, see benchmark/benchmarkcontainers.
template <class T>
class VectorFifo
{
public:
    void push(const T &value) { items.push_back(value); }
    T &front() { return items.front(); }
    const T &front() const { return items.front(); }
    void pop() { items.erase(items.begin()); }
    bool empty() const { return items.empty(); }
    size_t size() const { return items.size(); }
private:
    std::vector<T> items;
};
}

#endif
