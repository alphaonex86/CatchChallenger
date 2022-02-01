#ifndef CATCHCHALLENGER_QTPLAYERUPDATER_H
#define CATCHCHALLENGER_QTPLAYERUPDATER_H

#include <QTimer>
#include "../../base/PlayerUpdaterBase.hpp"
#include <stdint.h>

class QtPlayerUpdater : public QTimer, public CatchChallenger::PlayerUpdaterBase
{
public:
    explicit QtPlayerUpdater();
private:
    void exec();
    void setInterval(int ms);
};

#endif // QtPLAYERUPDATER_H
