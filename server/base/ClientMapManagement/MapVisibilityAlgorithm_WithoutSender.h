#ifndef CATCHCHALLENGER_MAPVISIBILITYALGORITHM_WITHOUTSENDER_H
#define CATCHCHALLENGER_MAPVISIBILITYALGORITHM_WITHOUTSENDER_H

#include <QObject>

namespace CatchChallenger {
class MapVisibilityAlgorithm_WithoutSender : public QObject
{
public:
    explicit MapVisibilityAlgorithm_WithoutSender();
    ~MapVisibilityAlgorithm_WithoutSender();
    static MapVisibilityAlgorithm_WithoutSender mapVisibilityAlgorithm_WithoutSender;
    QList<void*> allClient;
public:
    void generalPurgeBuffer();
};
}

#endif
