#ifndef CATCHCHALLENGER_MAPVISIBILITYALGORITHM_WITHOUTSENDER_H
#define CATCHCHALLENGER_MAPVISIBILITYALGORITHM_WITHOUTSENDER_H

#include <QObject>

namespace CatchChallenger {
class MapVisibilityAlgorithm_WithoutSender : public QObject
{
    Q_OBJECT
public:
    explicit MapVisibilityAlgorithm_WithoutSender();
    virtual ~MapVisibilityAlgorithm_WithoutSender();
    static MapVisibilityAlgorithm_WithoutSender mapVisibilityAlgorithm_WithoutSender;
    QList<void*> allClient;
public slots:
    void generalPurgeBuffer();
};
}

#endif
