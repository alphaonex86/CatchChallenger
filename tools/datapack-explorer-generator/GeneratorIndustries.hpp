#ifndef GENERATOR_INDUSTRIES_HPP
#define GENERATOR_INDUSTRIES_HPP

#include <string>
#include <cstdint>

namespace GeneratorIndustries {
    void generate();
    std::string relativePathForIndustry(const std::string &mainCode, const std::string &id);
}

#endif // GENERATOR_INDUSTRIES_HPP
