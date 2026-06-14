#ifndef GENERATOR_QUESTS_HPP
#define GENERATOR_QUESTS_HPP

#include <string>
#include <cstdint>

namespace GeneratorQuests {
    void generate();

    // Per-main quest page, old PHP scheme: quests/<mainCode>/<id>-<urlname>.html
    std::string relativePathForQuest(const std::string &mainCode, uint16_t id);

    // Compatibility overload: resolves the id against the LAST parsed main
    // that defines it (the only quest data the loader still holds when the
    // generators run). Callers that know the main should pass it explicitly.
    std::string relativePathForQuest(uint16_t id);
}

#endif // GENERATOR_QUESTS_HPP
