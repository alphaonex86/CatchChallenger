#ifndef GENERATOR_QUESTS_HPP
#define GENERATOR_QUESTS_HPP

#include <string>
#include <cstdint>

namespace GeneratorQuests {
    void generate();
    std::string relativePathForQuest(uint16_t id);
}

#endif // GENERATOR_QUESTS_HPP
