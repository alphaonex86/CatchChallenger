#ifndef JL2922_HPS_FLAT_SET_SERIALIZER_H
#define JL2922_HPS_FLAT_SET_SERIALIZER_H

#if __cplusplus >= 202302L || defined(__cpp_lib_flat_set)

#include <flat_set>
#include <utility>
#include <vector>
#include "../serializer.h"

namespace hps {

template <class T, class Compare, class KeyContainer, class B>
class Serializer<std::flat_set<T, Compare, KeyContainer>, B> {
 public:
  static void serialize(const std::flat_set<T, Compare, KeyContainer>& container, B& ob) {
    KeyContainer vec(container.begin(), container.end());
    Serializer<KeyContainer, B>::serialize(vec, ob);
  }

  static void parse(std::flat_set<T, Compare, KeyContainer>& container, B& ib) {
    KeyContainer vec;
    Serializer<KeyContainer, B>::parse(vec, ib);
    container = std::flat_set<T, Compare, KeyContainer>(
        std::sorted_unique, std::move(vec));
  }
};

}  // namespace hps

#endif
#endif
