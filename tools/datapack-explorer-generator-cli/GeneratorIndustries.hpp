#ifndef GENERATOR_INDUSTRIES_HPP
#define GENERATOR_INDUSTRIES_HPP

#include <string>
#include <cstdint>
#include <map>
#include <vector>

namespace GeneratorIndustries {
    struct IndustryResource { uint16_t item; uint32_t quantity; };
    struct IndustryProduct  { uint16_t item; uint32_t quantity; };

    struct IndustryData
    {
        std::string id;           // string ID from XML
        uint64_t time;
        uint32_t cycletobefull;
        std::vector<IndustryResource> resources;
        std::vector<IndustryProduct>  products;
    };

    void generate();
    std::string relativePathForIndustry(const std::string &mainCode, const std::string &id);
    // Parse map/main/<mainCode>/industries/industrialrecipe.xml, keyed by industry id
    std::map<std::string,IndustryData> parseIndustrialRecipe(const std::string &mainCode);
}

#endif // GENERATOR_INDUSTRIES_HPP
