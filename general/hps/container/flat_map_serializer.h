#ifndef JL2922_HPS_FLAT_MAP_SERIALIZER_H
#define JL2922_HPS_FLAT_MAP_SERIALIZER_H

#if __cplusplus >= 202302L || defined(__cpp_lib_flat_map)

#include <flat_map>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>
#include "../serializer.h"

namespace hps {

template <class K, class V, class Compare, class KeyContainer, class MappedContainer, class B>
class Serializer<std::flat_map<K, V, Compare, KeyContainer, MappedContainer>, B> {
 public:
  static void serialize(const std::flat_map<K, V, Compare, KeyContainer, MappedContainer>& container, B& ob) {
    Serializer<KeyContainer, B>::serialize(container.keys(), ob);
    Serializer<MappedContainer, B>::serialize(container.values(), ob);
  }

  static void parse(std::flat_map<K, V, Compare, KeyContainer, MappedContainer>& container, B& ib) {
    KeyContainer keys;
    MappedContainer values;
    Serializer<KeyContainer, B>::parse(keys, ib);
    Serializer<MappedContainer, B>::parse(values, ib);
    if (keys.size() != values.size()) {
      #ifdef __EXCEPTIONS
      throw std::runtime_error("hps flat_map parse: keys and values size mismatch");
      #else
      std::cerr << "hps flat_map parse: keys and values size mismatch" << std::endl;
      container.clear();
      return;
      #endif
    }
    container = std::flat_map<K, V, Compare, KeyContainer, MappedContainer>(
        std::move(keys), std::move(values));
  }
};

}  // namespace hps

#endif
#endif
