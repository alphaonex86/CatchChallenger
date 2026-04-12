#ifndef GENERATOR_ITEMS_HPP
#define GENERATOR_ITEMS_HPP

#include <string>
#include <cstdint>

namespace GeneratorItems {
    void generate();

    // Build a relative path for the item page.
    // Example: items/catchtrap/trap-grade-a.html
    // If the item image is catchtrap/trap-grade-a.png, subfolder is "catchtrap"
    // and stem is "trap-grade-a"; otherwise fallback is items/<id>.html.
    std::string relativePathForItem(uint16_t id);
}

#endif // GENERATOR_ITEMS_HPP
