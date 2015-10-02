#ifndef CATCHCHALLENGER_MAPVISIBILITYALGORITHM_WITHOUTSENDER_H
#define CATCHCHALLENGER_MAPVISIBILITYALGORITHM_WITHOUTSENDER_H

#include <std::vector>

namespace CatchChallenger {
class MapVisibilityAlgorithm_WithoutSender
{
public:
    explicit MapVisibilityAlgorithm_WithoutSender();
    ~MapVisibilityAlgorithm_WithoutSender();
    static MapVisibilityAlgorithm_WithoutSender mapVisibilityAlgorithm_WithoutSender;
public:
    void generalPurgeBuffer();
};
}

#endif
