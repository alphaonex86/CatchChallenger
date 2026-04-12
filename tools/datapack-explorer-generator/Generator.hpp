#ifndef GENERATOR_HPP
#define GENERATOR_HPP

#include <string>

// Base class/namespace for generators.
// Each generator produces HTML pages under the output folder.
namespace Generator {
    // Call all sub-generators in order. The datapack must already be loaded
    // into CatchChallenger::CommonDatapack::commonDatapack and
    // QtDatapackClientLoader::datapackLoader.
    void generateAll();
}

#endif // GENERATOR_HPP
