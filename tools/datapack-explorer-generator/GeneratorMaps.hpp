#ifndef GENERATOR_MAPS_HPP
#define GENERATOR_MAPS_HPP

#include <string>
#include <cstdint>
#include <cstddef>

namespace GeneratorMaps {
    void generate();

    // Return the HTML page path (relative to output root) for a given map.
    // The map is identified by (set index in MapStore, local map id).
    std::string relativePathForMapRef(size_t setIdx, size_t mapId);
}

#endif // GENERATOR_MAPS_HPP
