#ifndef GENERATOR_CRAFTING_HPP
#define GENERATOR_CRAFTING_HPP

#include <string>
#include <cstdint>

namespace GeneratorCrafting {
    void generate();
    std::string relativePathForCraftingRecipe(uint16_t id);
}

#endif // GENERATOR_CRAFTING_HPP
