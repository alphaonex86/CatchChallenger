#ifndef GENERATOR_MONSTERS_HPP
#define GENERATOR_MONSTERS_HPP

#include <string>
#include <cstdint>

namespace GeneratorMonsters {
    void generate();
    std::string relativePathForMonster(uint16_t id);
}

#endif // GENERATOR_MONSTERS_HPP
